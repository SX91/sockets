#include "Echo.h"

#include <cassert>
#include <iostream>

void TCPEchoClient::bind(const SocketAddress &address) {
    m_Socket.bind(address);
}

void TCPEchoClient::connect(const SocketAddress &remote) {
    m_Socket.connect(remote);
}

void TCPEchoClient::run() {
    std::string input;
    char buf[MAXSIZE];

    while (true) {
        input.clear();
        std::getline(std::cin, input);

        m_Socket.writeAll(input.c_str(), input.size() + 1);
        size_t amount = m_Socket.read(buf, sizeof(buf));

        if (amount == 0)
            break;

        std::cout << buf << std::endl;
    }
}

void UDPEchoClient::bind(const SocketAddress &address) {
    m_Socket.bind(address);
}

void UDPEchoClient::connect(const SocketAddress &remote) {
    m_RemoteAddress = remote;
}

void UDPEchoClient::run() {
    std::string input;
    char buf[MAXSIZE];

    while (true) {
        input.clear();
        std::getline(std::cin, input);

        assert(input.size() + 1 < MAXSIZE);

        m_Socket.sendTo(input.c_str(), input.size() + 1, m_RemoteAddress);

        while (true) {
            auto result = m_Socket.receiveFrom(buf, MAXSIZE);
            if (result.second == m_RemoteAddress)
                break;
        }

        std::cout << buf << std::endl;
    }
}
