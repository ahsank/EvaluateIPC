#pragma once
#include "framework.h"

static const char client_message[] = "Hello from client!";
static const char server_message[] = "Hello from server!";

class EchoServer: public EventHandler {
public:
    EchoServer() {
        description = "echo server";
    }
    virtual void process(char *data, int len, bool iseof) {
        if (strcmp(data, client_message) != 0) {
            ERROR_OUT("Invalid message from client:%s\n", data);
            exit(1);
        }
        INFO_OUT("Server sending response\n");
        send(server_message, sizeof(server_message), 1);
    }
};

class EchoClient: public EventHandler {
public:
    int numGot;
    int numSent;
    int maxSend;
    timeval beginTime;

    EchoClient(int nReq) :
        maxSend(nReq) {
        numSent = numGot = 0;
        description = "echo client";
    }
    void sendData() {
        INFO_OUT("Sending data %d\n", numSent);
        send(client_message, sizeof(client_message), true);
        numSent++;
    }
    virtual void process(char *data, int len, bool iseof) {
        if (strcmp(data, server_message) != 0) {
            ERROR_OUT("Invalid message from server:%s\n", data);
            exit(1);
        }
        numGot++;
        INFO_OUT("Client process response %d\n", numGot);
        if (numGot == 1) {
            printCurrentTime();
            gettimeofday(&beginTime, NULL);
        }
        if (numGot == maxSend) {
            timeval endTime;
            gettimeofday(&endTime, NULL);
            printCurrentTime();
            unsigned long timediff = getTimeDiff(&endTime, &beginTime);
            printf("Number of message %d, usec %ld, Number of message per sec %ld\n", maxSend,
                    timediff, maxSend*1000000UL / (timediff ? timediff : 1));

            getParent()->cancelLoop();
            return;
        }
        sendData();
    }

    virtual void enable() {
        INFO_OUT("Echo client enabled\n");
        sendData();
    }
};
