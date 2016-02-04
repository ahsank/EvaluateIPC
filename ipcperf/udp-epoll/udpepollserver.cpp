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
#include <sys/epoll.h>
#include <assert.h>
#include "framework.h"

// Main event loop
class UdpEpollMain: public EventMain {
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
        sockaddr_in dest;
    };

    struct MyEventData {
        int fd;
        EventHandler *pHandler;
    };
#define MAXEVENTS 64

    void process() {
        int efd;
        epoll_event event = {0};
        epoll_event events[MAXEVENTS];

        efd = epoll_create1(0);

        if (efd == -1) {
            perror("epoll_create");
            exit(1);
        }
        if (this->listener != -1) {
            MyEventData data = {listener, server};
            event.data.ptr = new MyEventData(data);
            event.events = EPOLLIN;
            if (epoll_ctl(efd, EPOLL_CTL_ADD, listener, &event) == -1) {
                perror("epoll_ctl");
                exit(1);
            }
        }
        if (this->dest != -1) {
            MyEventData data = {dest, client};
            event.data.ptr = new MyEventData(data);
            event.events = EPOLLIN;
            if (epoll_ctl(efd, EPOLL_CTL_ADD, dest, &event) == -1) {
                perror("epoll_ctl");
                exit(1);
            }
        }

        while (!loopEnd) {
            int nevents = epoll_wait(efd, events, MAXEVENTS, -1);
            for (int i=0; i < nevents; i++) {
                epoll_event *pev = &events[i];
                MyEventData *data = (MyEventData*)pev->data.ptr;
                if ((pev->events & EPOLLERR) ||
                       (pev->events & EPOLLHUP) ||
                       !(pev->events & EPOLLIN)) {
                    fprintf(stderr, "epoll error\n");
                    close(data->fd);
                    delete data;
                    continue;
                }
                INFO_OUT("Reading socket %d", i);
                char buf[1024];
                struct sockaddr_in si_from;
                unsigned int slen = sizeof(si_from);
                ssize_t  result = recvfrom(data->fd, buf, sizeof(buf), 0,
                                           (sockaddr*)&si_from, &slen );
                INFO_OUT("Done reading socket");
                if (result < 0) {
                	INFO_OUT("Read error");
                    perror("recv");
                    close(data->fd);
                    delete data;
                    continue;
                } else if (result == 0) {
                	INFO_OUT("Closing socket %d", i);
                    close(data->fd);
                    delete data;
                    break;
                }
                if (data->pHandler) {
                	INFO_OUT("Before process");
                    MyContext fromcontext = {data->fd, si_from};
                    data->pHandler->setContext((Context*) &fromcontext);
                    data->pHandler->process(buf, result, true);
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
        if (::sendto(pContext->fd, data, len, 0, (sockaddr*)&pContext->dest,
                      sizeof(pContext->dest)) == -1) {
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
        context.dest = sin;
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
UdpEpollMain epollMain;
EventMain *g_pmainProcessor = &epollMain;
#endif
