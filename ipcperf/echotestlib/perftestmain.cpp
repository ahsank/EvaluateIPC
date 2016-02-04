
#include "echotestlib.h"

const char *opt = "csp:a:";

class ArgParser {
public:
    bool isClientOnly;
    bool isServerOnly;
    const char *pAddress;
    const char *pPort;
    ArgParser() :
        isClientOnly(false),
        isServerOnly(false),
        pAddress("127.0.0.1"),
        pPort("8000") {

    }
    void parseArgs(int argc, char **argv) {
        int c;
        while ((c = getopt(argc, argv, opt)) != -1) {
            switch (c) {
            case 'c': isClientOnly = true; break;
            case 's': isServerOnly = true; break;
            case 'p': pPort = optarg; break;
            case 'a': pAddress = optarg; break;
            default:
                fprintf(stderr, "./eventserver [-cs] [-p port] [-a address]\n");
                exit(1);

            }
        }
    }

};

extern EventMain *g_pmainProcessor;

int main(int argc, char **argv) {
    EchoServer server;
    EchoClient client(1000);
    g_pmainProcessor->initialize();
    server.initialize();
    client.initialize();
    ArgParser argParser;
    argParser.parseArgs(argc, argv);
    if (!argParser.isClientOnly) {
        g_pmainProcessor->bindServer(argParser.pPort, &server);
    }
    if (!argParser.isServerOnly) {
        g_pmainProcessor->connectToServer(argParser.pAddress, argParser.pPort, &client);
    }
    g_pmainProcessor->process();

}
