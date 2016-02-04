#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "framework.h"
//
// Server and client in the same process communicating though copying
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


class MMapLoopMain: public EventMain {
protected:

    membuff *pbuff;
    EventHandler *server;
    EventHandler *client;
    bool loopEnd;
    key_t key;
    int fd;
    
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

    void createSharedMem(bool isServer) {
        fd = open("/tmp/mmapserver", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
        if (fd <= 0) {
            diep("open");
        }
        if (isServer) {
            char buff[SharedMemorySize];
            memset(&buff, 0, sizeof(buff));
            write(fd, buff, SharedMemorySize);
        }
        if (lseek(fd, SharedMemorySize-1, SEEK_SET ) == -1) {
            diep("lseek");
        }
        this->pbuff = (membuff *)mmap(NULL, SharedMemorySize,
                                PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (pbuff == (membuff*)MAP_FAILED) {
            diep("mmap");
        }

    }
    void bindServer(const char *port, EventHandler *pProcessor) {
        this->server = pProcessor;
        setParent(pProcessor);
        pProcessor->setContext((Context*) (void*) ClientDest);
        createSharedMem(true);
    }

    void send(EventHandler *p, const char *data, int len, bool isDataEnd) {
        if (!p) {
            INFO_OUT("Invalid context");
            return;
        }
        this->pbuff->len  = len;
        memcpy(this->pbuff->buffer, data, len);
        this->pbuff->destId = (uint16_t)(long)(void*)p->getContext();
        // msync(this->pbuff, SharedMemorySize, MS_SYNC|MS_INVALIDATE);

    }

    void connectToServer(const char *address, const char *port,
            EventHandler *pProcessor) {
        this->client = pProcessor;
        setParent(pProcessor);
        createSharedMem(false);
        // Put destination  as context
        pProcessor->setContext((Context*) (void*) ServerDest);
        pProcessor->enable();
    }
};


#ifdef BUILDTEST
MMapLoopMain mmaploopMain;
EventMain *g_pmainProcessor = &mmaploopMain;
#endif
