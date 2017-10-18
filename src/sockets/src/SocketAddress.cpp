#include "../include/SocketAddress.h"

#include <memory.h>
#include <stdexcept>

#include <arpa/inet.h>


SocketAddress::SocketAddress()
        : m_AddressStorage({})
        , m_AddressSize(0)
{}

SocketAddress::SocketAddress(const sockaddr_storage &address, socklen_t address_size)
        : m_AddressStorage(address)
        , m_AddressSize(address_size)
{}

bool SocketAddress::operator==(const SocketAddress &rhs) {
    if (m_AddressSize == rhs.m_AddressSize) {
        auto lhs_ptr = reinterpret_cast<const char *>(&m_AddressStorage);
        auto rhs_ptr = reinterpret_cast<const char *>(&rhs.m_AddressStorage);
        return memcmp(lhs_ptr, rhs_ptr, m_AddressSize) == 0;
    }

    return false;
}

bool SocketAddress::valid() const noexcept {
    return m_AddressSize > 0;
}

sa_family_t SocketAddress::getRawFamily() const noexcept {
    return m_AddressStorage.ss_family;
}

const sockaddr *SocketAddress::getRawAddress() const noexcept {
    return reinterpret_cast<const sockaddr *>(&m_AddressStorage);
}

socklen_t SocketAddress::getRawSize() const noexcept {
    return m_AddressSize;
}

namespace inet4 {

SocketAddress any(uint16_t port) {
    sockaddr_storage storage = {};

    auto sin = reinterpret_cast<sockaddr_in *>(&storage);
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = INADDR_ANY;
    sin->sin_port = htons(port);

    return {storage, sizeof(sockaddr_in)};
}

SocketAddress localhost(uint16_t port) {
    sockaddr_storage storage = {};

    auto sin = reinterpret_cast<sockaddr_in *>(&storage);
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = INADDR_LOOPBACK;
    sin->sin_port = htons(port);

    return {storage, sizeof(sockaddr_in)};
}

SocketAddress make_address(const std::string &ip, uint16_t port) {
    sockaddr_storage storage = {};
    auto address = reinterpret_cast<sockaddr_in *>(&storage);
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if (inet_pton(AF_INET, ip.data(), &address->sin_addr) == 0)
        throw std::invalid_argument("invalid IPv4 address");

    return {storage, sizeof(sockaddr_in)};
}

SocketAddress make_address(const std::pair<std::string, uint16_t> &address_pair) {
    return make_address(address_pair.first, address_pair.second);
}

SocketAddress make_address(const std::string &address) {
    size_t pos = address.find(':');
    if (pos == address.npos)
        return make_address(address, 0);

    std::string ip_str = address.substr(0, pos);
    std::string port_str = address.substr(pos + 1, address.npos);

    unsigned long port = 0;
    size_t processed = 0;
    try {
        port = std::stoul(port_str, &processed);
    } catch (std::invalid_argument &err) {
        throw std::invalid_argument("invalid port value in [IP:PORT] encoding");
    }

    if (processed != port_str.length() || port > UINT16_MAX)
        throw std::invalid_argument("address must be encoded as [IP] or [IP:PORT]");

    return make_address(ip_str, static_cast<uint16_t>(port));
}

std::pair<std::string, uint16_t> convert_address(const SocketAddress &address) {
    char buf[45];
    auto sin = reinterpret_cast<const sockaddr_in *>(address.getRawAddress());
    const char *end = inet_ntop(address.getRawFamily(), address.getRawAddress(), buf, 45);

    if (end != nullptr) {
        return {std::string(buf), sin->sin_port};
    }

    return {};
}

std::string address_to_string(const SocketAddress &address) {
    auto address_port_pair = convert_address(address);
    std::string out(address_port_pair.first);
    out += ":";
    out += std::to_string(address_port_pair.second);

    return out;
}

} //namespace inet4
