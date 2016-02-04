#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/select.h>
#include <assert.h>
#include "framework.h"

const int max_buff = 32767;

static const char client_message[] = "Hello from client!";
static const char server_message[] = "Hello from server!";
const char * multicast_address = "226.0.0.1";

class MulticastServer: public EventHandler {
public:
    int numMessages;

    MulticastServer() {
        description = "echo server";
        numMessages = 1;
    }

    virtual void process(char *data, int len, bool iseof) {
        if (strcmp(data, client_message) != 0) {
            ERROR_OUT("Invalid message from client:%s\n", data);
            exit(1);
        }
        INFO_OUT("Server sending response\n");
        for (int i=0; i < numMessages; i++){
            send(server_message, sizeof(server_message), 1);
        }
    }
};

class MulticastClient: public EventHandler {
public:
    int numGot;
    int numSent;
    int maxSend;
    timeval beginTime;

    MulticastClient(int nReq) :
        maxSend(nReq) {
        numSent = numGot = 0;
        description = "echo client";
    }
    void sendData() {
        INFO_OUT("Sending data %d\n", numSent);
        send(client_message, sizeof(client_message), true);
        numSent++;
    }
    virtual void process(char *data, int len, bool iseof) {
        if (strcmp(data, server_message) != 0) {
            ERROR_OUT("Invalid message from server:%s\n", data);
            exit(1);
        }
        numGot++;
        INFO_OUT("Messages received %d\n", numGot);
        if (numGot == 1) {
            printCurrentTime();
            gettimeofday(&beginTime, NULL);
        }
        if (numGot == maxSend) {
            timeval endTime;
            gettimeofday(&endTime, NULL);
            printCurrentTime();
            unsigned long timediff = getTimeDiff(&endTime, &beginTime);
            printf("Number of message %d, usec %ld, Number of message per sec %ld\n", maxSend,
                    timediff, maxSend*1000000UL / (timediff ? timediff : 1));

            getParent()->cancelLoop();
            return;
        }
    }

    virtual void enable() {
        INFO_OUT("Echo client enabled\n");
        sendData();
    }
};

struct ClientState {
    EventHandler *handler;
};

class UdpSelectMain: public EventMain {
protected:

    int         listenerSocket;
    int         clientSocket;
    int         multicastSendSocket;
    EventHandler* server;
    EventHandler* client;
    bool        loopEnd;
    ClientState* pStates;
    const char*  multicastPort;

    // Parameters for UDP send
    struct UdpParams {
        int socketfd;	    // File descriptor of the socket
        sockaddr_in dest;   // Destination address
    };

public:

    void setMulticastPort(const char *port)
    {
        this->multicastPort = port;
    }

    void initialize() {
        loopEnd = false;
        clientSocket = listenerSocket = multicastSendSocket = -1;

    }
    /**
     * Creates file descriptor array
     */
    void buildfds(ClientState ** states, int *fds, int *pnumfds) {
        int numfds = 0;
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (!states[i])
                continue;
            fds[numfds++] = i;
        }
        *pnumfds = numfds;

    }

    void process() {
        int fds[FD_SETSIZE];
        int numfds = 0;
        struct ClientState *states[FD_SETSIZE];
        int maxfd;
        fd_set readset, writeset, exset;
        UdpParams params = {};

        params.socketfd = multicastSendSocket;
        params.dest.sin_family = AF_INET;
        params.dest.sin_addr.s_addr = inet_addr(multicast_address);
        params.dest.sin_port = htons(atoi(this->multicastPort));

        for (int i = 0; i < FD_SETSIZE; ++i)
            states[i] = NULL;

        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_ZERO(&exset);
        if (clientSocket != -1) {
            struct ClientState *state = (ClientState*) malloc(
                    sizeof(struct ClientState));
            assert(state);
            state->handler = this->client;
            states[clientSocket] = state;
        }
        if (listenerSocket != -1) {
            struct ClientState *state = (ClientState*) malloc(
                    sizeof(struct ClientState));
            assert(state);
            state->handler = server;
            states[listenerSocket] = state;
            fds[numfds++] = listenerSocket;
        }
        maxfd = listenerSocket;
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (!states[i])
                continue;
            if (i > maxfd) {
                maxfd = i;
            }
            FD_SET(i, &readset);
        }
        buildfds(states, fds, &numfds);

        INFO_OUT("Listening socket %d", listenerSocket);
        INFO_OUT("Connected socket %d", clientSocket);

        while (!loopEnd) {

            FD_ZERO(&readset);
            FD_ZERO(&writeset);
            FD_ZERO(&exset);

            maxfd = listenerSocket;
            for (int i = 0; i < numfds; i++) {
                if (fds[i] > maxfd) {
                    maxfd = fds[i];
                }
                FD_SET(fds[i], &readset);
            }
            INFO_OUT("slecting %d sockets", numfds);
            int numResult;
            if ((numResult = select(maxfd + 1, &readset, NULL, NULL, NULL))
                    < 0) {
                perror("select");
                return;
            }
            INFO_OUT("selected %d sockets", numResult);

            for (int i = 0; i < maxfd + 1; ++i) {
                int r = 0;

                if (FD_ISSET(i, &readset)) {
                    char buf[1024];
                    ssize_t result;
                    struct sockaddr_in si_from;
                    unsigned int slen = sizeof(si_from);

                    while (1) {
                        INFO_OUT("Reading socket %d", i);
                        result = recvfrom(i, buf, sizeof(buf), 0,
                                (sockaddr*)&si_from, &slen);
                        if (result < 0) {
                            perror("recvfrom");
                            break;
                        } else if (result == 0) {

                            break;
                        }
                        if (states[i] && states[i]->handler) {
                            states[i]->handler->setContext((Context*) &params);
                            states[i]->handler->process(buf, result, true);
                        }
                        else {
                            INFO_OUT("No states in %d", i);
                        }
                        break;

                    }
                    r = (result == 0) || (result < 0 && result != EAGAIN);

                }

                if (r) {
                    INFO_OUT("Closing socket %d", i);
                    free(states[i]);
                    states[i] = NULL;
                    close(i);
                    buildfds(states, fds, &numfds);
                }
            }

        }
    }

    void cancelLoop() {
        loopEnd = true;
    }

    void bindServer(const char *port, EventHandler *pProcessor) {
        sockaddr_in saddr = { 0 };
        int         multicast_sock;
        int         oneval = 1;
        in_addr     iaddr = {0};
        int         mc_ttl = 1;

        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = 0;
        saddr.sin_port = htons(atoi(port));

        listenerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        this->server = pProcessor;
        setParent(pProcessor);
        fcntl(listenerSocket, F_SETFL, O_NONBLOCK);

        setsockopt(listenerSocket, SOL_SOCKET, SO_REUSEADDR, &oneval,
                sizeof(oneval));
        if (bind(listenerSocket, (struct sockaddr*) &saddr, sizeof(saddr)) < 0) {
            diep("bind");
            return;
        }
        INFO_OUT("Bound to port %s", port);

        // Create socket for multicast send
        multicast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (multicast_sock < 0) {
            diep("socket");
        }
        this->multicastSendSocket = multicast_sock;

    }

    void send(EventHandler *p, const char *data, int len, bool isDataEnd) {
        if ((p == NULL) || (p->getContext() == NULL)) {
            INFO_OUT("Invalid context");
            return;
        }
        UdpParams *udpParams = (UdpParams*) p->getContext();
        if (::sendto(udpParams->socketfd, data, len, 0,
                (sockaddr*) &udpParams->dest, sizeof(udpParams->dest)) == -1) {
            diep("send");
        }
        INFO_OUT("Done sending socket %d", udpParams->socketfd);

    }

    void connectToServer(const char *address, const char *port,
            EventHandler *pProcessor) {
        sockaddr_in saddr = { 0 };
        in_addr     iaddr = {0};
        int         multicast_sock;
        int         unicast_sock;
        int         status;
        int         oneval = 1;
        ip_mreq     imreq = {0};

        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(atoi(this->multicastPort));
        // saddr.sin_addr.s_addr = inet_addr("192.168.1.12");
        saddr.sin_addr.s_addr = INADDR_ANY;

        multicast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (multicast_sock < 0) {
            diep("socket");
        }

        setsockopt(multicast_sock, SOL_SOCKET, SO_REUSEADDR, &oneval,
                        sizeof(oneval));

        fcntl(multicast_sock, F_SETFL, O_NONBLOCK);

        // Bind to the socket
        if (bind(multicast_sock, (sockaddr*)&saddr, sizeof(saddr)) < 0){
            diep("bind");
        }
        // Set outgoing interface to default
        imreq.imr_multiaddr.s_addr = inet_addr(multicast_address);
        imreq.imr_interface.s_addr = INADDR_ANY; // use DEFAULT interface

        // JOIN multicast group on default interface
        if (setsockopt(multicast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                      (const void *)&imreq, sizeof(struct ip_mreq)) < 0) {
            diep("add membership");
        }
        int socketBufferSize = 1024*1024;
        if (setsockopt(multicast_sock, SOL_SOCKET, SO_RCVBUF,
                      (const void *)&socketBufferSize,
                      sizeof(socketBufferSize)) < 0) {
            diep("setsockopt SO_RCVBUF");
        }

        this->clientSocket = multicast_sock;
        this->client = pProcessor;
        // Set this event handler as parent of client processor
        setParent(pProcessor);

        // Server unicast address
        unicast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (unicast_sock < 0) {
            diep("socket");
        }

        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = inet_addr(address);
        saddr.sin_port = htons(atoi(port));

        // Enable the client which will send message to server to tell
        // the server to send multicast packet
        UdpParams udpParams = { unicast_sock, saddr };
        pProcessor->setContext((Context*) &udpParams);
        pProcessor->enable();
        /// Close the unicast socket
        close(unicast_sock);

    }

};

const char *opt = "csp:a:";

class ArgParser {
public:
    bool isClientOnly;
    bool isServerOnly;
    const char *pAddress;
    const char *pPort;
    const char *multicastPort;

    ArgParser() :
        isClientOnly(false),
        isServerOnly(false),
        pAddress("127.0.0.1"),
        pPort("8000"),
        multicastPort("8100")
    {

    }
    void parseArgs(int argc, char **argv) {
        int c;
        while ((c = getopt(argc, argv, opt)) != -1) {
            switch (c) {
            case 'c': isClientOnly = true; break;
            case 's': isServerOnly = true; break;
            case 'p': pPort = optarg; break;
            case 'm': multicastPort = optarg; break;
            case 'a': pAddress = optarg; break;
            default:
                fprintf(stderr, "./eventserver [-cs] [-p port] [-a address]\n");
                exit(1);

            }
        }
    }

};

UdpSelectMain udpSelectMain;
EventMain *g_pmainProcessor = &udpSelectMain;

int main(int argc, char **argv) {
    const int numMessages = 1000;
    MulticastServer server;
    server.numMessages = numMessages;
    MulticastClient client(numMessages);
    g_pmainProcessor->initialize();
    server.initialize();
    client.initialize();
    ArgParser argParser;
    argParser.parseArgs(argc, argv);
    udpSelectMain.setMulticastPort(argParser.multicastPort);

    if (!argParser.isClientOnly) {
        g_pmainProcessor->bindServer(argParser.pPort, &server);
    }
    if (!argParser.isServerOnly) {
        g_pmainProcessor->connectToServer(argParser.pAddress, argParser.pPort,
                &client);
    }
    g_pmainProcessor->process();

}

