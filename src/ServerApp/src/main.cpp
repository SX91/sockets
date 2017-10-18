#include <iostream>
#include <getopt.h>

#include "Reactor.h"
#include "SocketAddress.h"

#include "Echo.h"

void printHelp(char *progname) {
    char ws[] = "    ";
    std::cout << progname << " [options]" << " BINDADDR" << std::endl
              << ws << "-h | --help    print this message" << std::endl
              << ws << "-u | --udp     use UDP protocol instead of TCP" << std::endl;
}


int main(int argc, char **argv) {
    static const char short_options[] = "hu";
    static const option long_options[] = {
            {"help", no_argument, nullptr, 'h'},
            {"udp",  no_argument, nullptr, 'u'},
            {nullptr, 0,          nullptr, 0},
    };

    bool use_tcp = true;
    SocketAddress bind_address;

    while (true) {
        const auto opt = getopt_long(argc, argv, short_options, long_options, nullptr);

        if (-1 == opt)
            break;

        switch (opt) {
            case 'h':
                printHelp(argv[0]);
                exit(0);
            case 'u':
                use_tcp = false;
                break;
            case '?':
            default:
                std::cerr << "ERROR: "
                          << "invalid argument " << opt << std::endl;
                exit(1);
        }
    }

    if (argv[optind] == nullptr) {
        std::cerr << "ERROR: "
                  << "missing mandatory argument BINDADDR"
                  << std::endl;
        exit(1);
    } else {
        try {
            bind_address = inet4::make_address(std::string(argv[optind]));
        } catch (std::invalid_argument &err) {
            std::cerr << "ERROR: "
                      << "could not parse BINDADDR argument as [IP:PORT] ("
                      << err.what() << ")" << std::endl;
            exit(1);
        }
    }

    std::unique_ptr<IHandler> handler;
    ReactorPtr reactor = std::make_shared<Reactor>();

    try {
        if (use_tcp) {
            auto server = new TCPEchoServer(reactor);
            server->bind(bind_address);
            server->listen(128);
            handler.reset(server);
        } else {
            auto server = new UDPEcho(reactor);
            server->bind(bind_address);
            handler.reset(server);
        }

        reactor->exec();
    } catch (std::exception &err) {
        std::cerr << "ERROR: "
                  << err.what()
                  << std::endl;
        exit(1);
    }
    return 0;
}
