#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <assert.h>
#include "framework.h"
//
// Server and client in the same process communicating though copying
// data using memcpy. It is to test overhead of class infrastructure. On
// Intel i7 Mac OSX, there were 11M roundtrip messages/sec
//

const int max_buff = 32767;


class MemcpyLoopMain: public EventMain {
protected:

    char *buffer;
    int len;
    EventHandler *server;
    EventHandler *client;
    bool loopEnd;
    EventHandler *dest;

public:

    void initialize() {
        loopEnd = false;
        buffer = new char[max_buff];
        len = 0;
        server = client = dest = NULL;

    }
    void process() {

        while (!loopEnd) {
            if (len == 0) {
                continue;
            }
            if (dest) {
                EventHandler *context = NULL;
                context = (dest == client) ? server : client;
                dest->setContext((Context*) (void*) context);
                int tmplen = len;
                len = 0;
                dest->process(buffer, tmplen, true);
            }
        }

    }

    void cancelLoop() {
        loopEnd = true;
    }

    void bindServer(const char *port, EventHandler *pProcessor) {
        this->server = pProcessor;
        setParent(pProcessor);
    }

    void send(EventHandler *p, const char *data, int len, bool isDataEnd) {
        if (!p) {
            INFO_OUT("Invalid context");
            return;
        }
        memcpy(this->buffer, data, len);
        this->dest = (EventHandler *) p->getContext();
        this->len = len;

    }

    void connectToServer(const char *address, const char *port,
            EventHandler *pProcessor) {
        this->client = pProcessor;
        setParent(pProcessor);
        pProcessor->setContext((Context*) (void*) server);
        pProcessor->enable();
    }
};


#ifdef BUILDTEST
MemcpyLoopMain memcpyloopMain;
EventMain *g_pmainProcessor = &memcpyloopMain;
#endif
