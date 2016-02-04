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
class EpollMain: public EventMain {
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
            MyEventData data = {listener, NULL};
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
                if (listener == data->fd) {
                    struct sockaddr_storage ss;
                    socklen_t slen = sizeof(ss);
                    int acceptfd = accept(listener,  (struct sockaddr*) &ss, &slen);
                    if (acceptfd == -1) {
                        perror("accept");
                        continue;
                    }
                    fcntl(acceptfd, F_SETFL, O_NONBLOCK);
                    MyEventData adata = {acceptfd, server};
                    event.data.ptr = new MyEventData(adata);
                    event.events = EPOLLIN;
                    epoll_ctl(efd, EPOLL_CTL_ADD, acceptfd, &event);
                    continue;
                }
                INFO_OUT("Reading socket %d", i);
                char buf[1024];
                bool isDone = false;
                ssize_t  result = recv(data->fd, buf, sizeof(buf), 0);
                if (result < 0) {
                    perror("recv");
                    close(data->fd);
                    delete data;
                    continue;
                } else if (result == 0) {
                    close(data->fd);
                    delete data;
                    break;
                }
                if (data->pHandler) {
                    data->pHandler->setContext((Context*) (long) data->fd);
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
EpollMain EpollMain;
EventMain *g_pmainProcessor = &EpollMain;
#endif
