#ifndef PROJECT_ECHO_H
#define PROJECT_ECHO_H

#include <map>
#include <vector>

#include "BaseSocket.h"
#include "Handler.h"

const size_t BUF_SIZE = 65535;


class UDPEcho : public UDPSocketHandler {
public:
    explicit UDPEcho(ReactorWPtr reactor_ptr);

    void onReadReady(size_t amount) override;

    void onError(int code) override;
};


class TCPEcho : public TCPSocketHandler {
public:
    explicit TCPEcho(ReactorWPtr reactor_ptr);

    TCPEcho(TCPSocket &&socket, ReactorWPtr reactor_ptr);

    void onReadReady(size_t amount) override;

    void onConnect() override;

    void onDisconnect() override;

    void onError(int code) override;
};


class TCPEchoServer : public TCPServerHandler {
public:
    explicit TCPEchoServer(ReactorWPtr reactor_ptr);

    ~TCPEchoServer() override;

    void onConnectionPending() override;

    void onConnect() override;

    void onDisconnect() override;

    void onReadReady(size_t amount) override;

    void onError(int code) override;

protected:
    void closeConnections();

    void addClient(SocketDescr fd, TCPEcho &&socket);

    void removeClient(SocketDescr fd);

    virtual void reactorConnectionEventCallback(Event event, SocketDescr fd);

private:
    std::map<SocketDescr, TCPEcho> m_ClientMap;
};

#endif //PROJECT_ECHO_H
