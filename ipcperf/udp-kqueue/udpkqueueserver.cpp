#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/event.h>
#include <assert.h>
#include "framework.h"

// Main event loop
class UdpKQueueMain: public EventMain {
protected:

    int listener;
    int dest;
    EventHandler *server;
    EventHandler *client;
    bool loopEnd;

public:

    void initialize() {
        loopEnd = false;
        dest = listener = -1;

    }

    struct MyContext {
        int fd;
        sockaddr_in *pdest;
    };

#define MAXEVENTS 64

    void process() {
        int kqfd;
        struct kevent event = {0};
        struct kevent events[MAXEVENTS];

        kqfd = kqueue();
        if(kqfd == -1) {
            diep("kqueue");
        }
        
        if (this->listener != -1) {
            EV_SET(&event, this->listener, EVFILT_READ, EV_ADD, 0, 0, server);
            if(kevent(kqfd, &event, 1, NULL, 0, NULL) == -1) {
                diep("kevent");
            }
            INFO_OUT("Added listenner");
        }
        if (this->dest != -1) {
            EV_SET(&event, this->dest, EVFILT_READ, EV_ADD, 0, 0, client);
            if(kevent(kqfd, &event, 1, NULL, 0, NULL) == -1) {
                diep("kevent");
            }
        }

        while (!loopEnd) {
            int nevents = kevent(kqfd, NULL, 0, events, MAXEVENTS, NULL);
            INFO_OUT("Got event");
            if (nevents < 0) {
                diep("kevent main");
            }
            for (int i=0; i < nevents; i++) {
                struct kevent *pev = &events[i];
                
                if ((pev->flags & EV_ERROR) ) {
                    fprintf(stderr, "EVERROR %s\n", strerror(pev->data));
                    exit(1);
                }

                INFO_OUT("Reading socket %d", i);
                char buf[1024];
                struct sockaddr_in si_from;
                unsigned int slen = sizeof(si_from);
                ssize_t  result = recvfrom(pev->ident, buf, sizeof(buf), 0,
                                           (sockaddr*)&si_from, &slen );
                if (result < 0) {
                    perror("recv");
                    close(pev->ident);
                    continue;
                } else if (result == 0) {
                    close(pev->ident);
                    break;
                }
                if (pev->udata) {
                    INFO_OUT("Before process");
                    MyContext fromcontext = {pev->ident, &si_from};
                    EventHandler *pHandler = (EventHandler*)pev->udata;
                    pHandler->setContext((Context*) &fromcontext);
                    pHandler->process(buf, result, true);
                }
                
            }
        }
        close(listener);
    }

    void cancelLoop() {
        loopEnd = true;
    }

    void bindServer(const char *port, EventHandler *pProcessor) {
        struct sockaddr_in sin = { 0 };

        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = 0;
        sin.sin_port = htons(atoi(port));

        listener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        this->server = pProcessor;
        setParent(pProcessor);
        fcntl(listener, F_SETFL, O_NONBLOCK);
        int oneval = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &oneval, sizeof(oneval));
        if (bind(listener, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
            perror("bind");
            return;
        }
        INFO_OUT("Bound to port %s", port);
    }

    void send(EventHandler *p, const char *data, int len, bool isDataEnd) {
        MyContext *pContext;
        if (!p | !(pContext = (MyContext*) p->getContext())) {
            INFO_OUT("Invalid context");
            return;
        }
        INFO_OUT("Sending data to %d", pContext->fd);
        if (::sendto(pContext->fd, data, len, 0, (sockaddr*)pContext->pdest,
                      sizeof(*pContext->pdest)) == -1) {
            perror("send");
        }
        INFO_OUT("Done sending");

    }

    void connectToServer(const char *address, const char *port,
            EventHandler *pProcessor) {
        sockaddr_in sin = { 0 };
        MyContext context;

        sin.sin_family = AF_INET;
        sin.sin_port = htons(atoi(port));
        inet_pton(AF_INET, address, &(sin.sin_addr));
        int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        context.pdest = &sin;
        context.fd = sockfd;
        this->client = pProcessor;
        this->dest = sockfd;
        setParent(pProcessor);
        pProcessor->setContext((Context*) (void*) &context);
        fcntl(dest, F_SETFL, O_NONBLOCK);
        pProcessor->enable();

    }

};


#ifdef BUILDTEST
UdpKQueueMain udpkqueueMain;
EventMain *g_pmainProcessor = &udpkqueueMain;
#endif
