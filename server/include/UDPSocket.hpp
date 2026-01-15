#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <stdexcept>
#include <cstring>
#include <fcntl.h>

namespace RType::Network {

class UDPSocket {
public:
    UDPSocket() : _sockfd(-1) {}
    
    ~UDPSocket() {
        close();
    }
    
    void bind(uint16_t port) {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }
        int opt = 1;
        if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set SO_REUSEADDR");
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (::bind(_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            throw std::runtime_error("Failed to bind socket on port " + std::to_string(port));
        }
        
        _port = port;
    }
    
    void setNonBlocking(bool nonBlocking) {
        int flags = fcntl(_sockfd, F_GETFL, 0);
        if (nonBlocking) {
            fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK);
        } else {
            fcntl(_sockfd, F_SETFL, flags & ~O_NONBLOCK);
        }
    }
    
    ssize_t sendTo(const void* data, size_t size, const sockaddr_in& dest) {
        return sendto(_sockfd, data, size, 0, 
                      (struct sockaddr*)&dest, sizeof(dest));
    }
    
    ssize_t receiveFrom(void* buffer, size_t size, sockaddr_in& source) {
        socklen_t addr_len = sizeof(source);
        return recvfrom(_sockfd, buffer, size, 0, 
                        (struct sockaddr*)&source, &addr_len);
    }
    
    void close() {
        if (_sockfd >= 0) {
            ::close(_sockfd);
            _sockfd = -1;
        }
    }
    
    int getFd() const { return _sockfd; }
    uint16_t getPort() const { return _port; }

private:
    int _sockfd;
    uint16_t _port;
};


class Address {
public:
    sockaddr_in addr;
    
    Address() {
        std::memset(&addr, 0, sizeof(addr));
    }
    
    Address(const sockaddr_in& a) : addr(a) {}
    
    std::string toString() const {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);
        return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
    }
    
    bool operator==(const Address& other) const {
        return addr.sin_addr.s_addr == other.addr.sin_addr.s_addr &&
               addr.sin_port == other.addr.sin_port;
    }
    
    bool operator<(const Address& other) const {
        if (addr.sin_addr.s_addr != other.addr.sin_addr.s_addr)
            return addr.sin_addr.s_addr < other.addr.sin_addr.s_addr;
        return addr.sin_port < other.addr.sin_port;
    }
};

} // namespace RType::Network
