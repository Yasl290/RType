#pragma once

#include "network/ISocket.hpp"
#include "network/UDPAddress.hpp"

#ifdef _WIN32
    #define NOMINMAX
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    typedef int ssize_t;
#else
    #include <sys/socket.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

#include <stdexcept>
#include <cstring>

namespace RType::Network {

class UDPSocket : public ISocket {
public:
    UDPSocket() : _sockfd(-1), _port(0) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }
    
    ~UDPSocket() override {
        close();
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    void bind(uint16_t port) override {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
        if (_sockfd == INVALID_SOCKET) {
#else
        if (_sockfd < 0) {
#endif
            throw std::runtime_error("Failed to create socket");
        }
        
        int opt = 1;
#ifdef _WIN32
        if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
#else
        if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
            throw std::runtime_error("Failed to set SO_REUSEADDR");
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (::bind(_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
            ::closesocket(_sockfd);
            _sockfd = INVALID_SOCKET;
#else
            ::close(_sockfd);
            _sockfd = -1;
#endif
            throw std::runtime_error("Failed to bind socket on port " + std::to_string(port));
        }
        
        _port = port;
    }
    
    void setNonBlocking(bool nonBlocking) override {
#ifdef _WIN32
        u_long mode = nonBlocking ? 1 : 0;
        if (ioctlsocket(_sockfd, FIONBIO, &mode) != 0) {
            throw std::runtime_error("Failed to set socket non-blocking");
        }
#else
        int flags = fcntl(_sockfd, F_GETFL, 0);
        if (flags == -1) {
            throw std::runtime_error("Failed to get socket flags");
        }
        
        if (nonBlocking) {
            flags |= O_NONBLOCK;
        } else {
            flags &= ~O_NONBLOCK;
        }
        
        if (fcntl(_sockfd, F_SETFL, flags) == -1) {
            throw std::runtime_error("Failed to set socket non-blocking");
        }
#endif
    }
    
    ssize_t sendTo(const void* data, size_t size, const IAddress& dest) override {
        auto* udpAddr = dynamic_cast<const UDPAddress*>(&dest);
        if (!udpAddr) {
            throw std::runtime_error("Invalid address type for UDP socket");
        }
        
        const auto& nativeAddr = udpAddr->getNativeAddr();
#ifdef _WIN32
        return sendto(_sockfd, (const char*)data, (int)size, 0,
                     reinterpret_cast<const sockaddr*>(&nativeAddr),
                     sizeof(nativeAddr));
#else
        return sendto(_sockfd, data, size, 0,
                     reinterpret_cast<const sockaddr*>(&nativeAddr),
                     sizeof(nativeAddr));
#endif
    }
    
    ssize_t receiveFrom(void* buffer, size_t size, IAddress& source) override {
        auto* udpAddr = dynamic_cast<UDPAddress*>(&source);
        if (!udpAddr) {
            throw std::runtime_error("Invalid address type for UDP socket");
        }
        
        auto& nativeAddr = udpAddr->getNativeAddr();
        socklen_t addr_len = sizeof(nativeAddr);
        
#ifdef _WIN32
        return recvfrom(_sockfd, (char*)buffer, (int)size, 0,
                       reinterpret_cast<sockaddr*>(&nativeAddr),
                       &addr_len);
#else
        return recvfrom(_sockfd, buffer, size, 0,
                       reinterpret_cast<sockaddr*>(&nativeAddr),
                       &addr_len);
#endif
    }
    
    void close() override {
#ifdef _WIN32
        if (_sockfd != INVALID_SOCKET) {
            ::closesocket(_sockfd);
            _sockfd = INVALID_SOCKET;
        }
#else
        if (_sockfd >= 0) {
            ::close(_sockfd);
            _sockfd = -1;
        }
#endif
    }
    
    int getFd() const override { return _sockfd; }
    uint16_t getPort() const override { return _port; }

private:
    int _sockfd;
    uint16_t _port;
};

} // namespace RType::Network