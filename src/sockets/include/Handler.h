#ifndef SOCKETS_HANDLER_H
#define SOCKETS_HANDLER_H

#include <utility>

#include "Reactor.h"
#include "BaseSocket.h"


class IHandler {
public:
    virtual ~IHandler() = default;

    virtual void onReadReady(size_t amount) = 0;

    virtual void onError(int code) = 0;
};

class IEventHandler {
public:
    virtual ~IEventHandler() = default;

    virtual void reactorEventCallback(Event event, SocketDescr descr) = 0;
};

class IConnectionHandler {
public:
    virtual ~IConnectionHandler() = default;

    virtual void onConnectionPending() = 0;
};

class AbstractHandler : public IHandler, public IEventHandler {
public:
    explicit AbstractHandler(ReactorWPtr reactor_ptr);

    AbstractHandler(const AbstractHandler &) = default;

    AbstractHandler(AbstractHandler &&) = default;

    AbstractHandler &operator=(const AbstractHandler &) = default;

    AbstractHandler &operator=(AbstractHandler &&) = default;

    ReactorWPtr getReactorPtr() const noexcept;

private:
    ReactorWPtr m_Reactor;
};

class UDPSocketHandler : public AbstractHandler {
public:
    explicit UDPSocketHandler(ReactorWPtr reactor_ptr);

    ~UDPSocketHandler() override;

    void bind(const SocketAddress &address);

    void sendDatagram(const ByteArray &data, const SocketAddress &destination);

    std::pair<ByteArray, SocketAddress> receiveDatagram();;

protected:
    void registerCallback();

    void unregisterCallback();

    void reactorEventCallback(Event event, SocketDescr fd) override;

private:
    UDPSocket m_Socket;
};

class TCPSocketHandler : public AbstractHandler {
public:
    explicit TCPSocketHandler(ReactorWPtr reactor_ptr);

    TCPSocketHandler(TCPSocket &&socket, ReactorWPtr reactor_ptr);

    ~TCPSocketHandler() override;

    TCPSocketHandler(TCPSocketHandler &&) = default;

    TCPSocketHandler &operator=(TCPSocketHandler &&) = default;

    void bind(const SocketAddress &address);

    void connect(const SocketAddress &address);

    void send(const ByteArray &data);

    ByteArray receive(size_t max_size);

    size_t bytesAvailable() const;

    virtual void onConnect() = 0;

    virtual void onDisconnect() = 0;

    const TCPSocket &getSocket() const noexcept;

protected:
    TCPSocket &getSocket() noexcept;

    void registerCallback();

    void unregisterCallback();

    void reactorEventCallback(Event event, SocketDescr fd) override;

private:
    TCPSocket m_Socket;
};

class TCPServerHandler : public IConnectionHandler, public TCPSocketHandler {
public:
    explicit TCPServerHandler(ReactorWPtr reactor_ptr);

    TCPServerHandler(TCPSocket &&socket, ReactorWPtr reactor_ptr);

    ~TCPServerHandler() override = default;

    void listen(int max_clients);

    TCPSocket nextPendingConnection();

protected:
    void reactorEventCallback(Event event, SocketDescr fd) override;

private:
    bool m_Listening = false;
};


#endif
