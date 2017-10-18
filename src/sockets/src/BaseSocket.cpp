#include "BaseSocket.h"
#include "Error.h"

#include <arpa/inet.h>
#include <cassert>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>


BaseSocket::BaseSocket(SocketDescr fd)
        : m_SocketFd(fd)
{
    assert(fd > 0 && "invalid socket descriptor");
}

BaseSocket::~BaseSocket() {
    close();
}

BaseSocket::BaseSocket(BaseSocket &&rhs) noexcept
        : m_SocketFd(rhs.m_SocketFd)
{
    rhs.m_SocketFd = 0;
}

BaseSocket &BaseSocket::operator=(BaseSocket &&rhs) noexcept {
    m_SocketFd = rhs.m_SocketFd;
    rhs.m_SocketFd = 0;
    return *this;
}

void BaseSocket::close() noexcept {
    if (m_SocketFd) {
        ::close(m_SocketFd);
        m_SocketFd = 0;
    }
}

void BaseSocket::bind(const SocketAddress &address) {
    int ok = ::bind(m_SocketFd, address.getRawAddress(), address.getRawSize());
    if (ok == SOCKET_ERR)
        throw SocketError("could not bind socket to address", errno);
}

size_t BaseSocket::bytesAvailable() const {
    int available;
    if (ioctl(m_SocketFd, FIONREAD, &available) == -1)
        throw SocketError("ioctl error", errno);

    return (size_t) available;
}

void BaseSocket::connect(const SocketAddress &address) {
    int ok = ::connect(getRawSocket(), address.getRawAddress(), address.getRawSize());
    if (ok == SOCKET_ERR)
        throw SocketError("could not connect to remote peer", errno);
}

ByteArray BaseSocket::read(size_t max_size) {
    ByteArray buf(max_size);

    size_t amount = read(buf.data(), max_size);
    buf.resize(amount);
    buf.shrink_to_fit();

    return buf;
}

size_t BaseSocket::read(char *data, size_t data_size) {
    ssize_t amount = ::read(m_SocketFd, data, data_size);
    if (amount < 0)
        throw SocketError("could not read from socket", errno);

    return (size_t) amount;
}

size_t BaseSocket::write(const char *data, size_t data_size) {
    ssize_t amount = ::write(m_SocketFd, data, data_size);
    if (amount == SOCKET_ERR)
        throw SocketError("could not write to socket", errno);

    return (size_t) amount;
}

void BaseSocket::writeAll(const char *data, size_t data_size) {
    size_t total_sent = 0;

    while (total_sent != data_size)
        total_sent += write(&data[total_sent], data_size - total_sent);
}

void BaseSocket::writeAll(const ByteArray &array) {
    writeAll(array.data(), array.size());
}

SocketDescr BaseSocket::getRawSocket() const noexcept {
    return m_SocketFd;
}

void BaseSocket::setReuseAddress() {
    int optval = 1;
    if (setsockopt(m_SocketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        throw SocketError("could not set socket options", errno);
}

void BaseSocket::setNonBlocking() {
    int flags = fcntl(m_SocketFd, F_GETFL, 0);
    if (flags == -1)
        throw SocketError("could not get flags for fd", errno);

    if (fcntl(m_SocketFd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw SocketError("could not set flags to fd", errno);
}

BaseSocket::BaseSocket()
        : m_SocketFd(0)
{}

void BaseSocket::setRawSocket(SocketDescr fd) noexcept {
    m_SocketFd = fd;
}

UDPSocket::UDPSocket() {
    SocketDescr sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == SOCKET_ERR)
        throw SocketError("could not create socket", errno);

    setRawSocket(sock);
}

size_t UDPSocket::sendTo(const char *data, size_t data_size, const SocketAddress &address) {
    ssize_t sent = ::sendto(getRawSocket(), data, data_size, 0,
                            address.getRawAddress(), address.getRawSize());

    if (sent == -1)
        throw SocketError("could not send dgram to peer", errno);

    return static_cast<size_t>(sent);
}

size_t UDPSocket::sendTo(const ByteArray &data, const SocketAddress &address) {
    return sendTo(data.data(), data.size(), address);
}

std::pair<size_t, SocketAddress> UDPSocket::receiveFrom(char *data, size_t data_size) {
    sockaddr_storage storage = {};
    socklen_t storage_size = sizeof(storage);

    ssize_t amount = ::recvfrom(getRawSocket(), data, data_size, 0,
                                reinterpret_cast<sockaddr *>(&storage), &storage_size);

    if (amount == SOCKET_ERR)
        throw SocketError("could not receive datagram", errno);

    return {static_cast<size_t>(amount), SocketAddress(storage, storage_size)};
}

std::pair<ByteArray, SocketAddress> UDPSocket::receiveFrom() {
    size_t dgam_size = bytesAvailable();
    ByteArray buf(dgam_size);

    std::pair<size_t, SocketAddress> result = receiveFrom(buf.data(), buf.max_size());

    return {std::move(buf), result.second};
}

TCPSocket::TCPSocket() {
    SocketDescr sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == SOCKET_ERR)
        throw SocketError("could not create socket", errno);

    setRawSocket(sock);
}

TCPSocket::TCPSocket(SocketDescr fd, const SocketAddress &remote)
        : BaseSocket(fd)
{}

TCPSocket TCPSocket::accept() {
    sockaddr_storage storage = {};
    socklen_t storage_len;

    SocketDescr new_sock = ::accept(getRawSocket(), (sockaddr *) &storage, &storage_len);
    if (new_sock == SOCKET_ERR)
        throw SocketError("could not accept new connection", errno);

    return TCPSocket(new_sock, {storage, storage_len});
}

void TCPSocket::listen(int max_clients) {
    if (::listen(getRawSocket(), max_clients) == SOCKET_ERR)
        throw SocketError("could not start listening", errno);
}
