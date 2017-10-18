#include "Echo.h"

#include <iostream>
#include <algorithm>
#include <numeric>
#include <valarray>
#include <Error.h>

namespace {

template<typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &rhs) {
    bool not_first = false;

    os << '[';
    for (auto &v : rhs) {
        if (not_first) {
            os << ", ";
        } else {
            not_first = true;
        }
        os << v;
    }

    return os << ']';
}

}
std::ostream &coutByteArray(std::ostream &os, const ByteArray &arr) noexcept {
    if (arr.back() == '\0') {
        return std::cout << arr.data();
    } else {
        return std::cout.write(arr.data(), arr.size());
    }
}

std::vector<int> filterAndConvertDigits(const std::vector<char> &arr) {
    std::vector<int> out;

    for (auto &c : arr) {
        if (std::isdigit(c))
            out.push_back(static_cast<int>(c - '0'));
    }

    return out;
}

void printDigits(const ByteArray &ba) {
    std::vector<int> digits = filterAndConvertDigits(ba);

    if (!digits.empty()) {
        int sum = std::accumulate(digits.begin(), digits.end(), 0);
        std::sort(digits.rbegin(), digits.rend());
        std::cout << "sum: " << sum << std::endl;
        std::cout << "rsorted: " << digits << std::endl;
        std::cout << "min: " << digits.back()  << "; "
                  << "max: " << digits.front() << std::endl;
    }
}

UDPEcho::UDPEcho(ReactorWPtr reactor_ptr)
        : UDPSocketHandler(std::move(reactor_ptr)) {}

void UDPEcho::onReadReady(size_t amount) {
    auto pair = receiveDatagram();
    ByteArray &data = pair.first;
    SocketAddress &address = pair.second;

    coutByteArray(std::cout, data) << std::endl;
    printDigits(data);

    // echo!
    sendDatagram(data, address);
}

void UDPEcho::onError(int code) {}

TCPEcho::TCPEcho(ReactorWPtr reactor_ptr)
        : TCPSocketHandler(std::move(reactor_ptr)) {}

TCPEcho::TCPEcho(TCPSocket &&socket, ReactorWPtr reactor_ptr)
        : TCPSocketHandler(std::move(socket), std::move(reactor_ptr)) {}

void TCPEcho::onReadReady(size_t amount) {
    ByteArray data = receive(BUF_SIZE);

    coutByteArray(std::cout, data) << std::endl;
    printDigits(data);

    send(data);
}

void TCPEcho::onConnect() {}

void TCPEcho::onDisconnect() {}

void TCPEcho::onError(int code) {}

TCPEchoServer::TCPEchoServer(ReactorWPtr reactor_ptr)
        : TCPServerHandler(std::move(reactor_ptr)) {}

TCPEchoServer::~TCPEchoServer() {
    closeConnections();
}

void TCPEchoServer::onConnectionPending() {
    auto new_socket = nextPendingConnection();
    new_socket.setNonBlocking();
    SocketDescr fd = new_socket.getRawSocket();
    addClient(fd, {std::move(new_socket), getReactorPtr()});
}

void TCPEchoServer::onConnect() {}

void TCPEchoServer::onDisconnect() {}

void TCPEchoServer::onReadReady(size_t amount) {}

void TCPEchoServer::onError(int code) {}

void TCPEchoServer::closeConnections() {
    if (auto reactor = getReactorPtr().lock()) {
        for (auto &client_pair : m_ClientMap)
            reactor->unsubscribe(client_pair.first);

        m_ClientMap.clear();
    }
}

void TCPEchoServer::addClient(SocketDescr fd, TCPEcho &&socket) {
    using namespace std::placeholders;

    m_ClientMap.emplace(fd, std::move(socket));
    if (auto reactor = getReactorPtr().lock()) {
        auto handler = std::bind(&TCPEchoServer::reactorConnectionEventCallback, this, _1, _2);
        reactor->subscribe(fd, Ready::readable() | Ready::hup(),
                           PollOpt::edge(), std::move(handler));
    }
}

void TCPEchoServer::removeClient(SocketDescr fd) {
    if (auto reactor = getReactorPtr().lock())
        reactor->unsubscribe(fd);

    m_ClientMap.erase(fd);
}

void TCPEchoServer::reactorConnectionEventCallback(Event event, SocketDescr fd) {
    auto iter = m_ClientMap.find(fd);

    if (iter != m_ClientMap.end()) {
        size_t amount = 0;
        TCPSocketHandler &socket = iter->second;

        try {
            switch (event) {
                case Event::ReadReady:
                    amount = socket.bytesAvailable();
                    if (amount == 0) {
                        socket.onDisconnect();
                        removeClient(fd);
                    } else {
                        socket.onReadReady(amount);
                    }
                    break;
                case Event::HangUp:
                    socket.onDisconnect();
                    removeClient(fd);
                    break;
                case Event::Error:
                    socket.onError(errno);
                    break;
                default:
                    break;
            }
        } catch (SocketError &err) {
            std::cerr << "ERROR: "
                      << err.what() << std::endl;
            std::cerr << "SERVER: client is removed due to socket error" << std::endl;
            removeClient(fd);
        }
    }
}
