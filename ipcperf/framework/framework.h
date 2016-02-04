#pragma once
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include <arpa/inet.h>

#ifndef traceLevel
#define traceLevel 0
#endif


inline void diep(const char *s)
{
    perror(s);
    exit(1);
}

#define dieif(cond, s) if ((cond)) { diep(s);}

// Behaves similarly to printf(...), but adds file, line, and function
// information. I omit do ... while(0) because I always use curly braces in my
// if statements.
#define INFO_OUT(...) if (traceLevel){\
printf("%s:%d: %s():\t", __FILE__, __LINE__, __FUNCTION__);\
printf(__VA_ARGS__);\
putc('\n', stdout); \
    fflush(stdout); \
}

// Behaves similarly to fprintf(stderr, ...), but adds file, line, and function
// information.
#define ERROR_OUT(...) {\
fprintf(stderr, "\033[0;1m%s:%d: %s():\t", __FILE__, __LINE__, __FUNCTION__);\
fprintf(stderr, __VA_ARGS__);\
fprintf(stderr, "\e[0m");\
    fflush(stderr); \
}

// Behaves similarly to perror(...), but supports printf formatting and prints
// file, line, and function information.
#define ERRNO_OUT(...) {\
fprintf(stderr, "\033[0;1m%s:%d: %s():\t", __FILE__, __LINE__, __FUNCTION__);\
fprintf(stderr, __VA_ARGS__);\
fprintf(stderr, ": %d (%s)\e[0m\n", errno, strerror(errno));\
    fflush(stderr);\
}

inline long int getTimeDiff(struct timeval *t2, struct timeval *t1)
{
    return (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
}

inline void printCurrentTime()
{
    char buffer[30];
    time_t curtime;
    timeval currTime;

    gettimeofday(&currTime, NULL);
    curtime = currTime.tv_sec;
    strftime(buffer, 30, "%m-%d-%Y  %T", localtime(&curtime));
    printf("%s.%06ld\n", buffer, (long)currTime.tv_usec);
}


class Processor {

public:

    Processor() :
            parent(NULL) {
    }

    virtual void initialize() {
    }

    virtual void process() {
    }

    virtual void enable() {
    }

    virtual ~Processor() {
    }

protected:

    Processor *parent;
    friend class EventMain;

};



struct Context;

class EventMain;

class EventHandler: public Processor {
protected:
    char *m_output;
    size_t m_outputLength;
    bool isMore;
    Context *pContext;
    const char *description;

public:
    EventHandler() :
            m_output(NULL), m_outputLength(0), isMore(false), pContext(NULL), description(
                    "generic") {
    }
    const char *getDescription() {
        return description;
    }

    void setContext(Context *pContext) {
        this->pContext = pContext;
    }

    Context * getContext() {
        return pContext;
    }

    void send(const char *data, int len, bool iseof);
    virtual void process(char *data, int length, bool iseof) = 0;

    EventMain *getParent() {
        return (EventMain*) this->parent;
    }

};


class EventMain: public Processor {

public:
    virtual void cancelLoop() = 0;
    virtual void bindServer(const char *port, EventHandler *pProcessor) = 0;
    virtual void send(EventHandler *p, const char *data, int len, bool isDataEnd) = 0;
    virtual void connectToServer(const char *address, const char *port,
            EventHandler *pProcessor) = 0;
    void setParent(EventHandler *pProcessor) {
        pProcessor->parent = this;
    }

};

inline void EventHandler::send(const char *data, int len, bool iseof) {
    ((EventMain*) this->parent)->send(this, data, len, iseof);
}

