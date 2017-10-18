#include "Handler.h"

#include <cassert>


AbstractHandler::AbstractHandler(ReactorWPtr reactor_ptr)
        : m_Reactor(std::move(reactor_ptr))
{}

ReactorWPtr AbstractHandler::getReactorPtr() const noexcept {
    return m_Reactor;
}

UDPSocketHandler::UDPSocketHandler(ReactorWPtr reactor_ptr)
        : AbstractHandler(std::move(reactor_ptr))
{}

UDPSocketHandler::~UDPSocketHandler() {
    unregisterCallback();
}

void UDPSocketHandler::bind(const SocketAddress &address) {
    m_Socket.bind(address);
    registerCallback();
}

void UDPSocketHandler::sendDatagram(const ByteArray &data, const SocketAddress &destination) {
    m_Socket.sendTo(data, destination);
}

std::pair<ByteArray, SocketAddress> UDPSocketHandler::receiveDatagram() {
    return m_Socket.receiveFrom();
}

void UDPSocketHandler::registerCallback() {
    using namespace std::placeholders;

    m_Socket.setNonBlocking();
    if (auto reactor = getReactorPtr().lock()) {
        auto handler = std::bind(&UDPSocketHandler::reactorEventCallback, this, _1, _2);
        Ready interest = Ready::readable() | Ready::writable() | Ready::hup();
        reactor->subscribe(m_Socket.getRawSocket(), interest, PollOpt::edge(), std::move(handler));
    }
}

void UDPSocketHandler::unregisterCallback() {
    if (auto reactor = getReactorPtr().lock())
        reactor->unsubscribe(m_Socket.getRawSocket());
}

void UDPSocketHandler::reactorEventCallback(Event event, SocketDescr fd) {
    size_t amount = 0;
    switch (event) {
        case Event::ReadReady:
            amount = m_Socket.bytesAvailable();
            this->onReadReady(amount);
            break;
        case Event::Error:
            this->onError(errno);
            break;
        default:
            break;
    }
}

TCPSocketHandler::TCPSocketHandler(ReactorWPtr reactor_ptr)
        : AbstractHandler(std::move(reactor_ptr))
{}

TCPSocketHandler::TCPSocketHandler(TCPSocket &&socket, ReactorWPtr reactor_ptr)
        : AbstractHandler(std::move(reactor_ptr))
        , m_Socket(std::move(socket))
{}

TCPSocketHandler::~TCPSocketHandler() {
    unregisterCallback();
}

void TCPSocketHandler::bind(const SocketAddress &address) {
    m_Socket.setReuseAddress();
    m_Socket.bind(address);
}

void TCPSocketHandler::connect(const SocketAddress &address) {
    m_Socket.connect(address);

    registerCallback();
}

void TCPSocketHandler::send(const ByteArray &data) {
    m_Socket.writeAll(data);
}

ByteArray TCPSocketHandler::receive(size_t max_size) {
    return m_Socket.read(max_size);
}

size_t TCPSocketHandler::bytesAvailable() const {
    return m_Socket.bytesAvailable();
}

const TCPSocket &TCPSocketHandler::getSocket() const noexcept {
    return m_Socket;
}

TCPSocket &TCPSocketHandler::getSocket() noexcept {
    return m_Socket;
}

void TCPSocketHandler::registerCallback() {
    using namespace std::placeholders;

    m_Socket.setNonBlocking();
    if (auto reactor = getReactorPtr().lock()) {
        auto handler = std::bind(&TCPSocketHandler::reactorEventCallback, this, _1, _2);
        Ready interest = Ready::readable() | Ready::writable() | Ready::hup();
        reactor->subscribe(m_Socket.getRawSocket(), interest, PollOpt::edge(), std::move(handler));
    }
}

void TCPSocketHandler::unregisterCallback() {
    if (auto reactor = getReactorPtr().lock())
        reactor->unsubscribe(m_Socket.getRawSocket());
}

void TCPSocketHandler::reactorEventCallback(Event event, SocketDescr fd) {
    size_t amount = 0;
    switch (event) {
        case Event::ReadReady:
            amount = m_Socket.bytesAvailable();
            if (amount == 0) {
                unregisterCallback();
                this->onDisconnect();
            } else {
                this->onReadReady(amount);
            }
            break;
        case Event::WriteReady:
            this->onConnect();
        case Event::HangUp:
            unregisterCallback();
            this->onDisconnect();
            break;
        case Event::Error:
            this->onError(errno);
            break;
        default:
            break;
    }
}

TCPServerHandler::TCPServerHandler(ReactorWPtr reactor_ptr)
        : TCPSocketHandler(std::move(reactor_ptr))
{}

TCPServerHandler::TCPServerHandler(TCPSocket &&socket, ReactorWPtr reactor_ptr)
        : TCPSocketHandler(std::move(socket), std::move(reactor_ptr)) {}

void TCPServerHandler::listen(int max_clients) {
    assert(max_clients > 0);
    getSocket().listen(max_clients);
    m_Listening = true;
    registerCallback();
}

TCPSocket TCPServerHandler::nextPendingConnection() {
    TCPSocket sock = getSocket().accept();
    return sock;
}

void TCPServerHandler::reactorEventCallback(Event event, SocketDescr fd) {
    size_t amount = 0;
    switch (event) {
        case Event::ReadReady:
            if (m_Listening) {
                onConnectionPending();
            } else {
                amount = getSocket().bytesAvailable();
                if (amount == 0) {
                    this->onDisconnect();
                    m_Listening = false;
                } else {
                    this->onReadReady(amount);
                }
            }
            break;
        case Event::HangUp:
            this->onDisconnect();
            m_Listening = false;
            break;
        case Event::Error:
            this->onError(errno);
            break;
        default:
            break;
    }
}
