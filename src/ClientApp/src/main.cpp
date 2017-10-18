#include <cassert>
#include <iostream>
#include <getopt.h>
#include <iomanip>

#include "SocketAddress.h"

#include "Echo.h"

void printHelp(char *progname) {
    char ws[] = "    ";
    std::cout << progname << " [options]" << " REMOTEADDR" << std::endl
              << ws << "-h | --help    print this message" << std::endl
              << ws << "-u | --udp     use UDP protocol instead of TCP" << std::endl
              << ws << "-b | --bind    address to bind to in IP:PORT format" << std::endl;
}


int main(int argc, char **argv) {
    static const char short_options[] = "hub:";
    static const option long_options[] = {
            {"help", no_argument,       nullptr, 'h'},
            {"udp",  no_argument,       nullptr, 'u'},
            {"bind", optional_argument, nullptr, 'b'},
            {nullptr, 0,                nullptr, 0},
    };

    bool use_tcp = true;
    SocketAddress bind_address = inet4::make_address("127.0.0.1", 0);
    SocketAddress server_address;

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
            case 'b':
                try {
                    bind_address = inet4::make_address(std::string(optarg));
                } catch (std::invalid_argument &err) {
                    std::cerr << "ERROR: "
                              << "could not parse BIND argument as [IP:PORT] ("
                              << err.what() << ")" << std::endl;
                    exit(1);
                }
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
                  << "missing mandatory argument REMOTE"
                  << std::endl;
        exit(1);
    } else {
        try {
            server_address = inet4::make_address(std::string(argv[optind]));
        } catch (std::invalid_argument &err) {
            std::cerr << "ERROR: "
                      << "could not parse REMOTE argument as [IP:PORT] ("
                      << err.what() << ")" << std::endl;
            exit(1);
        }
    }

    IService *client;
    if (use_tcp) {
        client = new TCPEchoClient();
    } else {
        client = new UDPEchoClient();
    }

    try {
        client->bind(bind_address);
        client->connect(server_address);

        client->run();
    } catch (std::exception &err) {
        std::cerr << "ERROR: "
                  << err.what()
                  << std::endl;
        exit(1);
    }

    return 0;
}
