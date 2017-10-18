#ifndef PROJECT_ECHO_H
#define PROJECT_ECHO_H

#include <cstddef>

#include "BaseSocket.h"

const size_t MAXSIZE = 65535;


class IService {
public:
    virtual ~IService() = default;

    virtual void bind(const SocketAddress &address) = 0;

    virtual void connect(const SocketAddress &address) = 0;

    virtual void run() = 0;
};

class TCPEchoClient : public IService {
public:
    ~TCPEchoClient() override = default;

    void bind(const SocketAddress &address) override;

    void connect(const SocketAddress &remote) override;

    void run() override;

private:

    TCPSocket m_Socket;
};

class UDPEchoClient : public IService {
public:
    ~UDPEchoClient() override = default;

    void bind(const SocketAddress &address) override;

    void connect(const SocketAddress &remote) override;

    void run() override;

private:
    SocketAddress m_RemoteAddress;
    UDPSocket m_Socket;
};

#endif //PROJECT_ECHO_H
