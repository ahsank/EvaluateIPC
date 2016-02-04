#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <iostream>
#include <sys/time.h>
#include <arpa/inet.h>
#include "framework.h"

class LibEventMain;

const int max_buff = 32767;

class LibEventMain: public EventMain {
protected:
    event_base *m_ebase;

public:

    void initialize() {
        if ((m_ebase = event_base_new()) == NULL) {
            perror("Cannot initialize libevent");
        }

    }
    void process() {
        event_base_dispatch(m_ebase);
    }

    void cancelLoop() {
        if (event_base_loopexit(m_ebase, NULL)) {
            ERROR_OUT("Error shutting down the server\n");
        }
    }

    static void acceptfn(int socket, short event, void *arg);
    static void readfn(bufferevent *bev, void *arg);
    static void errorfn(bufferevent *bev, short error, void *arg);

    void bindServer(const char *port, EventHandler *pProcessor) {
        sockaddr_in sin = { 0 };

        sin.sin_family = AF_INET;
        sin.sin_port = htons(atoi(port));

        int listenerfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenerfd == -1) {
            ERRNO_OUT("Error creating listening socket");
            exit(1);
        }
        int reuse = 1;
        if (setsockopt(listenerfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                sizeof(reuse))) {
            ERRNO_OUT("Error enabling socket reuse");
            exit(1);
        }
        evutil_make_socket_nonblocking(listenerfd);

        if (bind(listenerfd, (sockaddr*) &sin, sizeof(sin)) < 0) {
            perror("bind");
        }

        if (listen(listenerfd, 16) < 0) {
            perror("listen");
            return;
        }
        setParent(pProcessor);

        event *e = event_new(m_ebase, listenerfd, EV_READ | EV_PERSIST,
                acceptfn, (void*) pProcessor);

        event_add(e, NULL);

        INFO_OUT("Bound to port:%s\n", port);

    }

    void send(EventHandler *p, const char *data, int len, bool isDataEnd) {
        if (!data || !len) {
            return;
        }
        bufferevent *bev = (bufferevent *) p->getContext();
        if (!bev) {
            return;
        }
        bufferevent_write(bev, data, len);
        //bufferevent_flush(bev, EV_WRITE, BEV_NORMAL);

    }

    void connectToServer(const char *address, const char *port,
            EventHandler *pProcessor) {
        sockaddr_in sin = { 0 };

        sin.sin_family = AF_INET;
        sin.sin_port = htons(atoi(port));
        inet_pton(AF_INET, address, &(sin.sin_addr));

        // Investigate: set reuse address and make socket nonblocking?

        bufferevent *bev = bufferevent_socket_new(m_ebase, -1,
                BEV_OPT_CLOSE_ON_FREE);

        bufferevent_setcb(bev, readfn, NULL, errorfn, (void*) pProcessor);
        pProcessor->setContext((Context*) bev);
        setParent(pProcessor);
        if (bufferevent_socket_connect(bev, (struct sockaddr *) &sin,
                sizeof(sin)) < 0) {
            ERROR_OUT("Cannot bind to port %s", port);
            /* Error starting connection */
            bufferevent_free(bev);
            exit(1);
        }

    }

};

void LibEventMain::readfn(bufferevent *bev, void *arg) {
    EventHandler *p = (EventHandler *) arg;
    INFO_OUT("Readfn %s:\n", (p ? p->getDescription() : "None"));
    evbuffer *input, *output;

    size_t n;
    int i;

    input = bufferevent_get_input(bev);

    char buffer[max_buff];

//
//	char *data = evbuffer_readln(input, &n, EVBUFFER_EOL_LF);
//	p->process(data, n, true);
//	free(data);
    if ((n = evbuffer_remove(input, buffer, sizeof(buffer))) > 0) {
        p->process(buffer, n, !n);
    }

    // Todo: What happens to the remaining buffer that is less than max_buff
}

void LibEventMain::errorfn(bufferevent *bev, short int error, void *arg) {
    INFO_OUT("Errorfn: %x\n", error);
    if (error & BEV_EVENT_CONNECTED) {
        bufferevent_setwatermark(bev, EV_READ, 0, max_buff);
        bufferevent_enable(bev, EV_READ | EV_WRITE);
        EventHandler *p = (EventHandler *) arg;
        if (p) {
            p->enable();
        }
    }
    // if error & BEV_EVENT_EOF, BEV_EVENT_ERROR, BEV_EVENT_TIMEOUT
    if ((error & BEV_EVENT_ERROR) || (error & BEV_EVENT_EOF)
            || (error & BEV_EVENT_TIMEOUT)) {
        bufferevent_free(bev);
    }
}

void LibEventMain::acceptfn(int listener, short event, void *arg) {
    EventHandler *processor = (EventHandler*) arg;
    LibEventMain *plevent = (LibEventMain*) processor->getParent();

    sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    // Investigate: Using a loop to accept connections makes it faster?
    int fd = accept(listener, (sockaddr*) &ss, &slen);
    if (fd < 0) {
        // Investigate: Should check EWOULDBLOCK and EAGAIN?
        perror("accept");
        return;
    }
    INFO_OUT("Client connected on fd %d\n", fd);

    bufferevent *bev;
    evutil_make_socket_nonblocking(fd);
    bev = bufferevent_socket_new(plevent->m_ebase, fd, BEV_OPT_CLOSE_ON_FREE);
    processor->setContext((Context*) bev);
    bufferevent_setcb(bev, readfn, NULL, errorfn, arg);
    bufferevent_setwatermark(bev, EV_READ, 0, max_buff);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

}

#ifdef BUILDTEST
LibEventMain libEventMain;
EventMain *g_pmainProcessor = &libEventMain;
#endif
