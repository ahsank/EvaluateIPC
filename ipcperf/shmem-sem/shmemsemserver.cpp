#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <assert.h>
#include <sys/shm.h>
#include <sys/fcntl.h>
#include <semaphore.h>
#include "framework.h"
//
// Server and client communicating though copying
// data using shared memory and synchronizing using semaphore
//

const int SharedMemorySize = 512;

const int ClientDest = 1;
const int ServerDest = 2;
const char *ClientSemName = "/semclient";
const char *ServerSemName = "/semserver";

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
    sem_t *semServer;
    sem_t *semClient;
    sem_t *semNext;
    bool loopEnd;
    key_t key;
    int shmemid;

public:

    void initialize() {
        loopEnd = false;
        pbuff = NULL;
        server = client = NULL;
        semServer = semClient = semNext = NULL;

    }
    void process() {

        while (!loopEnd) {
        	if (sem_wait(semNext) == -1) {
        		diep("sem_wait");
        	}
            if (!pbuff || !pbuff->len) {
                ERROR_OUT("Invalid buffer");
                exit(1);
            }
            EventHandler *dest = (pbuff->destId == ClientDest) ? client : server;
            if (dest == NULL) {
            	ERROR_OUT("Invalid dest");
            	exit(1);
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
        semServer = sem_open(ServerSemName, O_CREAT, 0644, 0);
        semClient = sem_open(ClientSemName, O_CREAT, 0644, 0);
        if (this->server) {
        	semNext = semServer;
        }
        else  {
        	semNext = semClient;
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
        sem_t* semDest =  (p == this->server) ? semClient : semServer;
        if (sem_post(semDest) == -1) {
        	diep("sem_post");
        }
        if (semDest == semServer) {
        	if (this->server) {
        		semNext = semServer;
        	}
        	else {
        		semNext = semClient;
        	}
        } else {
        	if (this->client) {
        		semNext = semClient;
        	} else {
        		semNext = semServer;
        	}

        }

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
