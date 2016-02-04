#include <zmq.h>
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
// data using shared memory.
//

const int MaxSize = 512;

class ZeromqLoopMain: public EventMain {
protected:

    EventHandler *server;
    EventHandler *client;
    bool loopEnd;
    void* context;

public:

    void initialize() {
        loopEnd = false;
        server = client = NULL;
        context = NULL;

    }
    void process() {
        char buffer[MaxSize];
        zmq_pollitem_t items[2];
        memset(items, 0, sizeof(items));
        int nitems = 0;
        if (client) {
        	items[nitems].socket = (void*)client->getContext();
        	items[nitems].events = ZMQ_POLLIN;
        	nitems++;
        }
        if (server) {
        	items[nitems].socket = (void*)server->getContext();
        	items[nitems].events = ZMQ_POLLIN;
        	nitems++;
        }
        while (!loopEnd) {
        	for (int i=0; i < nitems; i++) {
        		items[i].revents = 0;
        	}
        	int rc = zmq_poll(items, nitems, -1);
        	if (rc < 0) {
        		diep("zmq_poll");
        	}
        	for (int i=0; i < nitems; i++) {
        		if (items[i].revents & ZMQ_POLLIN) {
        			int nbytes = zmq_recv(items[i].socket, buffer, sizeof(buffer), 0);
        			if (nbytes < 0) {
        				diep("zmq_recv");
        			}
        			EventHandler *processor = (i==0 && client) ? client : server;
        			processor->process(buffer, nbytes, true);
        			break;
        		}

        	}
        }

    }

    void cancelLoop() {
        loopEnd = true;
    }

    void createChannel(bool isServer, const char *port, EventHandler *pProcessor) {
        setParent(pProcessor);
    	context = zmq_ctx_new();
    	char path[256];
    	sprintf(path, "tcp://127.0.0.1:%s", port);
    	if (isServer) {
    		void *responder = zmq_socket(context, ZMQ_REP);
    		int rc = zmq_bind(responder, path);
    		if (rc != 0) {
    			diep("zmq_bind");
    		}
    		pProcessor->setContext((Context*)responder);
            this->server = pProcessor;
    	}
    	else {
    		void *requester = zmq_socket(context, ZMQ_REQ);
    		int rc = zmq_connect(requester, path);
    		if (rc != 0) {
    			diep("zmq_connect");
    		}
    		pProcessor->setContext((Context*)requester);
    		this->client = pProcessor;
    	}

    }
    void bindServer(const char *port, EventHandler *pProcessor) {
        createChannel(true, port, pProcessor);
    }

    void send(EventHandler *p, const char *data, int len, bool isDataEnd) {
        if (!p) {
            INFO_OUT("Invalid context");
            return;
        }
        int rc = zmq_send((void*)p->getContext(), data, len, 0);
        if (rc == -1) {
        	diep("zmq_send");
        }

    }

    void connectToServer(const char *address, const char *port,
            EventHandler *pProcessor) {
        createChannel(false, port, pProcessor);
        pProcessor->enable();
    }
};


#ifdef BUILDTEST
ZeromqLoopMain zmqloopMain;
EventMain *g_pmainProcessor = &zmqloopMain;
#endif
