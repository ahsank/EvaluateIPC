#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <assert.h>
#include <sys/shm.h>
#include "framework.h"
//
// Server and client communicating though copying
// data using shared memory.
//

const int SharedMemorySize = 512;

const int ClientDest = 1;
const int ServerDest = 2;


struct membuff{
    uint16_t destId;
    uint16_t len;
    char buffer[SharedMemorySize-2*sizeof(uint16_t)];
} __attribute__((packed));


class ShMemLoopMain: public EventMain {
protected:

    membuff *pbuff;
    EventHandler *server;
    EventHandler *client;
    bool loopEnd;
    key_t key;
    int shmemid;

public:

    void initialize() {
        loopEnd = false;
        pbuff = NULL;
        server = client = NULL;

    }
    void process() {

        while (!loopEnd) {
            if (!pbuff || !pbuff->len) {
                continue;
            }
            EventHandler *dest = (pbuff->destId == ClientDest) ? client : server;
            if (dest == NULL) {
                continue;
            }
            dest->process(pbuff->buffer, pbuff->len, true);
        }

    }

    void cancelLoop() {
        loopEnd = true;
    }

    void createSharedMem() {
        if ((key = ftok("/tmp", 'R')) == -1) {
            diep("ftok");
        }
        if ((shmemid = shmget(key, SharedMemorySize, 0644 | IPC_CREAT)) == -1) {
            diep("shmemget");
        }
        pbuff = (membuff *)shmat(shmemid, (void*)0, 0);
        if (pbuff == (membuff*)-1) {
            diep("shmat");
        }

    }
    void bindServer(const char *port, EventHandler *pProcessor) {
        this->server = pProcessor;
        setParent(pProcessor);
        pProcessor->setContext((Context*) (void*) ClientDest);
        createSharedMem();
    }

    void send(EventHandler *p, const char *data, int len, bool isDataEnd) {
        if (!p) {
            INFO_OUT("Invalid context");
            return;
        }
        this->pbuff->len  = len;
        memcpy(this->pbuff->buffer, data, len);
        this->pbuff->destId = (uint16_t)(long)(void*)p->getContext();

    }

    void connectToServer(const char *address, const char *port,
            EventHandler *pProcessor) {
        this->client = pProcessor;
        setParent(pProcessor);
        createSharedMem();
        // Put destination  as context
        pProcessor->setContext((Context*) (void*) ServerDest);
        pProcessor->enable();
    }
};


#ifdef BUILDTEST
ShMemLoopMain shMemloopMain;
EventMain *g_pmainProcessor = &shMemloopMain;
#endif
