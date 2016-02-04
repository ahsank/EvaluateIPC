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

struct ClientState {
    EventHandler *handler;
};

class SelectMain: public EventMain {
protected:

    int listener;
    int dest;
    EventHandler *server;
    EventHandler *client;
    bool loopEnd;
    ClientState *pStates;

public:

    void initialize() {
        loopEnd = false;
        dest = listener = -1;

    }
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
        for (int i = 0; i < FD_SETSIZE; ++i)
            states[i] = NULL;

        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_ZERO(&exset);
        if (dest != -1) {
            struct ClientState *state = (ClientState*) malloc(
                    sizeof(struct ClientState));
            assert(state);
            state->handler = this->client;
            states[dest] = state;
        }
        if (listener != -1) {
            FD_SET(listener, &readset);
        }
        maxfd = listener;
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (!states[i])
                continue;
            if (i > maxfd) {
                maxfd = i;
            }
            FD_SET(i, &readset);
        }
        buildfds(states, fds, &numfds);

        INFO_OUT("Listening socket %d", listener);
        INFO_OUT("Connected socket %d", dest);

        while (!loopEnd) {

            FD_ZERO(&readset);
            FD_ZERO(&writeset);
            FD_ZERO(&exset);

            if (listener != -1) {
                FD_SET(listener, &readset);
            }
            maxfd = listener;
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
            if (listener != -1 && FD_ISSET(listener, &readset)) {
                struct sockaddr_storage ss;
                socklen_t slen = sizeof(ss);
                int fd = accept(listener, (struct sockaddr*) &ss, &slen);
                if (fd < 0) {
                    perror("accept");
                } else if (fd > FD_SETSIZE) {
                    close(fd);
                } else {
                    fcntl(fd, F_SETFL, O_NONBLOCK);
                    struct ClientState *state = (ClientState*) malloc(
                            sizeof(struct ClientState));
                    assert(state);
                    state->handler = server;
                    states[fd] = state;
                    fds[numfds++] = fd;
                    INFO_OUT("Accepted socket %d", fd);

                }

            }

            for (int i = 0; i < maxfd + 1; ++i) {
                int r = 0;
                if (i == listener)
                    continue;

                if (FD_ISSET(i, &readset)) {
                    char buf[1024];
                    ssize_t result;

                    while (1) {
                        INFO_OUT("Reading socket %d", i);
                        result = recv(i, buf, sizeof(buf), 0);
                        if (result < 0) {
                            perror("recv");
                            break;
                        } else if (result == 0) {
                            break;
                        }
                        if (states[i] && states[i]->handler) {
                            states[i]->handler->setContext((Context*) (long) i);
                            states[i]->handler->process(buf, result, true);
                        }
                        break;

                    }
                    r = (result == 0) || (result < 0 && result != EAGAIN);

                }
                //if (r == 0 && FD_ISSET(i, &writeset)) {
                //r = do_write(i, state[i]);
                //}
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
        struct sockaddr_in sin = { 0 };

        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = 0;
        sin.sin_port = htons(atoi(port));

        listener = socket(AF_INET, SOCK_STREAM, 0);
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
        if (listen(listener, 16) < 0) {
            perror("listen");
            return;
        }
        INFO_OUT("Listenning to port %s", port);
    }

    void send(EventHandler *p, const char *data, int len, bool isDataEnd) {
        if (!p) {
            INFO_OUT("Invalid context");
            return;
        }
        if (::send((int) (long) p->getContext(), data, len, 0) == -1) {
            perror("send");
        }
        INFO_OUT("Done sending");

    }

    void connectToServer(const char *address, const char *port,
            EventHandler *pProcessor) {
        sockaddr_in sin = { 0 };

        sin.sin_family = AF_INET;
        sin.sin_port = htons(atoi(port));
        inet_pton(AF_INET, address, &(sin.sin_addr));
        dest = socket(AF_INET, SOCK_STREAM, 0);
        this->client = pProcessor;
        if (connect(dest, (sockaddr*) &sin, sizeof(sin)) < 0) {
            perror("connect");
            exit(1);
            return;
        }
        setParent(pProcessor);
        pProcessor->setContext((Context*) (long) dest);
        fcntl(dest, F_SETFL, O_NONBLOCK);
        pProcessor->enable();

        // Investigate: should set reuse address ?

    }

};


#ifdef BUILDTEST
SelectMain selectMain;
EventMain *g_pmainProcessor = &selectMain;
#endif
