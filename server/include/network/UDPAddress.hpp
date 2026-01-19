#pragma once

#include "network/ISocket.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#include <cstring>

namespace RType::Network {

class UDPAddress : public IAddress {
public:
    UDPAddress() {
        std::memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
    }
    
    explicit UDPAddress(const sockaddr_in& addr) : _addr(addr) {}
    
    UDPAddress(const std::string& ip, uint16_t port) {
        std::memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &_addr.sin_addr);
    }
    
    std::string toString() const override {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &_addr.sin_addr, ip, INET_ADDRSTRLEN);
        return std::string(ip) + ":" + std::to_string(ntohs(_addr.sin_port));
    }
    
    bool equals(const IAddress& other) const override {
        auto* udpOther = dynamic_cast<const UDPAddress*>(&other);
        if (!udpOther) return false;
        
        return _addr.sin_addr.s_addr == udpOther->_addr.sin_addr.s_addr &&
               _addr.sin_port == udpOther->_addr.sin_port;
    }
    
    std::unique_ptr<IAddress> clone() const override {
        return std::make_unique<UDPAddress>(_addr);
    }
    
    size_t hash() const override {
        return std::hash<uint32_t>{}(_addr.sin_addr.s_addr) ^
               (std::hash<uint16_t>{}(_addr.sin_port) << 1);
    }
    
    const sockaddr_in& getNativeAddr() const { return _addr; }
    sockaddr_in& getNativeAddr() { return _addr; }

private:
    sockaddr_in _addr;
};

struct AddressHash {
    size_t operator()(const std::shared_ptr<IAddress>& addr) const {
        return addr ? addr->hash() : 0;
    }
};

struct AddressEqual {
    bool operator()(const std::shared_ptr<IAddress>& a,
                   const std::shared_ptr<IAddress>& b) const {
        if (!a || !b) return a == b;
        return a->equals(*b);
    }
};

} // namespace RType::Network