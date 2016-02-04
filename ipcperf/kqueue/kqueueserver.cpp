#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <fcntl.h>
#include <assert.h>
#include "framework.h"


// Main event loop
class KQueueMain: public EventMain {
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

#define MAXEVENTS 64

    void process() {
        int kqfd;
        struct kevent event = {0};
        struct kevent events[MAXEVENTS];

        kqfd = kqueue();
        dieif(kqfd == -1, "kqueue");

        if (this->listener != -1) {

            EV_SET(&event, this->listener, EVFILT_READ, EV_ADD, 0, 0, 0);
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
            INFO_OUT("Added client");
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
                if (listener == pev->ident) {
                    struct sockaddr_storage ss;
                    socklen_t slen = sizeof(ss);
                    int acceptfd = accept(listener,  (struct sockaddr*) &ss, &slen);
                    if (acceptfd == -1) {
                        perror("accept");
                        continue;
                    }
                    fcntl(acceptfd, F_SETFL, O_NONBLOCK);
                    EV_SET(&event, acceptfd, EVFILT_READ, EV_ADD, 0, 0, server);
                    if(kevent(kqfd, &event, 1, NULL, 0, NULL) == -1) {
                        diep("kevent");
                    }
                    INFO_OUT("Added accept event");
                    continue;
                }
                INFO_OUT("Reading socket %d", i);
                char buf[1024];
                bool isDone = false;
                ssize_t  result = recv(pev->ident, buf, sizeof(buf), 0);
                if (result < 0) {
                    perror("recv");
                    close(pev->ident);
                    continue;
                } else if (result == 0) {
                    close(pev->ident);
                    break;
                }
                if (pev->udata) {
                    EventHandler *pHandler = (EventHandler*)pev->udata;
                    pHandler->setContext((Context*) (long) pev->ident);
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
KQueueMain kqmain;
EventMain *g_pmainProcessor = &kqmain;
#endif
