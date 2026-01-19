#include "network/ISocket.hpp"
#include "network/UDPSocket.hpp"

namespace RType::Network {

std::unique_ptr<ISocket> SocketFactory::create(Type type) {
    switch (type) {
        case Type::UDP:
            return std::make_unique<UDPSocket>();
        case Type::TCP:
            throw std::runtime_error("TCP not implemented yet");
        default:
            throw std::runtime_error("Unknown socket type");
    }
}

} // namespace RType::Network