#ifndef SOCKETS_SOCKETADDRESS_H
#define SOCKETS_SOCKETADDRESS_H

#include <sys/socket.h>
#include <string>
#include <utility>


class SocketAddress {
public:
    SocketAddress();

    SocketAddress(const sockaddr_storage &address, socklen_t address_size);

    SocketAddress(const SocketAddress &) = default;

    SocketAddress(SocketAddress &&) = default;

    SocketAddress &operator=(const SocketAddress &) = default;

    SocketAddress &operator=(SocketAddress &&) = default;

    bool operator==(const SocketAddress &rhs);

    bool valid() const noexcept;

    sa_family_t getRawFamily() const noexcept;

    const sockaddr *getRawAddress() const noexcept;

    socklen_t getRawSize() const noexcept;

private:
    sockaddr_storage m_AddressStorage;
    socklen_t m_AddressSize;
};


namespace inet4 {

SocketAddress any(uint16_t port = 0);

SocketAddress localhost(uint16_t port = 0);

SocketAddress make_address(const std::string &ip, uint16_t port);

SocketAddress make_address(const std::pair<std::string, uint16_t> &address_pair);

SocketAddress make_address(const std::string &address);

std::pair<std::string, uint16_t> convert_address(const SocketAddress &address);

std::string address_to_string(const SocketAddress &address);
}


#endif //SOCKETS_SOCKETADDRESS_H
