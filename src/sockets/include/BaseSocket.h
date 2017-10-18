#ifndef SOCKETS_BASESOCKET_H
#define SOCKETS_BASESOCKET_H

#include <vector>

#include "Reactor.h"
#include "SocketAddress.h"


using ByteArray = std::vector<char>;


class BaseSocket {
public:
    BaseSocket(const BaseSocket &) = delete;

    BaseSocket &operator=(const BaseSocket &) = delete;

    explicit BaseSocket(SocketDescr fd);

    virtual ~BaseSocket();

    BaseSocket(BaseSocket &&rhs) noexcept;

    BaseSocket &operator=(BaseSocket &&rhs) noexcept;

    void close() noexcept;

    void bind(const SocketAddress &address);

    size_t bytesAvailable() const;

    void connect(const SocketAddress &address);

    ByteArray read(size_t max_size);

    size_t read(char *data, size_t data_size);

    size_t write(const char *data, size_t data_size);

    void writeAll(const char *data, size_t data_size);

    void writeAll(const ByteArray &array);

    SocketDescr getRawSocket() const noexcept;

    // todo: replace with setOption(const Option &option, bool value)
    void setReuseAddress();

    void setNonBlocking();

protected:
    BaseSocket();

    void setRawSocket(SocketDescr fd) noexcept;

private:
    SocketDescr m_SocketFd;
};

class UDPSocket : public BaseSocket {
public:
    UDPSocket();

    explicit UDPSocket(SocketDescr fd) : BaseSocket(fd) {}

    size_t sendTo(const char *data, size_t data_size, const SocketAddress &address);

    size_t sendTo(const ByteArray &data, const SocketAddress &address);

    std::pair<size_t, SocketAddress> receiveFrom(char *data, size_t data_size);

    std::pair<ByteArray, SocketAddress> receiveFrom();
};

class TCPSocket : public BaseSocket {
public:
    TCPSocket();

    TCPSocket(SocketDescr fd, const SocketAddress &remote);

    TCPSocket accept();

    void listen(int max_clients);
};

#endif //SOCKETS_BASESOCKET_H
