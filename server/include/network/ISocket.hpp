#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>

#ifdef _WIN32
    typedef int ssize_t;
#endif

namespace RType::Network {

class IAddress {
public:
    virtual ~IAddress() = default;
    virtual std::string toString() const = 0;
    virtual bool equals(const IAddress& other) const = 0;
    virtual std::unique_ptr<IAddress> clone() const = 0;
    virtual size_t hash() const = 0;
};

class ISocket {
public:
    virtual ~ISocket() = default;
    
    virtual void bind(uint16_t port) = 0;
    virtual void setNonBlocking(bool nonBlocking) = 0;
    virtual void close() = 0;
    
    virtual ssize_t sendTo(const void* data, size_t size, const IAddress& dest) = 0;
    virtual ssize_t receiveFrom(void* buffer, size_t size, IAddress& source) = 0;
    
    virtual int getFd() const = 0;
    virtual uint16_t getPort() const = 0;
};

class SocketFactory {
public:
    enum class Type {
        UDP,
        TCP
    };
    
    static std::unique_ptr<ISocket> create(Type type);
};

}