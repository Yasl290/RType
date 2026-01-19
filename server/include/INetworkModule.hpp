#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <chrono>
#include <string>
#include <type_traits>

namespace RType {
namespace Server {
    class Client;
}
}

namespace RType::Network {

class IAddress;

struct ReceivedMessage {
    uint32_t client_id;
    std::shared_ptr<IAddress> source_addr;
    uint8_t packet_type;
    std::vector<uint8_t> payload;
    size_t payload_size;
};

class INetworkModule {
public:
    virtual ~INetworkModule() = default;

    virtual bool start(uint16_t port) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

    virtual std::vector<ReceivedMessage> pollMessages() = 0;

    template<typename T>
    void sendToClient(uint32_t client_id, const T& packet) {
        static_assert(std::is_trivially_copyable_v<T>, 
                     "Packet must be trivially copyable");
        sendToClientRaw(client_id, &packet, sizeof(T));
    }

    template<typename T>
    void broadcast(const T& packet) {
        static_assert(std::is_trivially_copyable_v<T>,
                     "Packet must be trivially copyable");
        broadcastRaw(&packet, sizeof(T), 0);
    }

    template<typename T>
    void broadcastExcept(uint32_t exclude_id, const T& packet) {
        static_assert(std::is_trivially_copyable_v<T>,
                     "Packet must be trivially copyable");
        broadcastRaw(&packet, sizeof(T), exclude_id);
    }

    template<typename T>
    void sendToAddress(const std::shared_ptr<IAddress>& addr, const T& packet) {
        static_assert(std::is_trivially_copyable_v<T>,
                     "Packet must be trivially copyable");
        sendToAddressRaw(addr, &packet, sizeof(T));
    }

    virtual std::vector<uint32_t> getConnectedClients() const = 0;
    virtual size_t getClientCount() const = 0;
    virtual void disconnectClient(uint32_t client_id) = 0;

    virtual bool hasAvailableSlot() const = 0;
    virtual uint8_t findFreePlayerSlot() const = 0;

    virtual uint32_t registerClient(const std::shared_ptr<IAddress>& addr, 
                                    const std::string& name, 
                                    uint8_t slot) = 0;

    virtual void unregisterClient(uint32_t client_id) = 0;

    virtual std::vector<uint32_t> checkTimeouts(std::chrono::seconds timeout) = 0;
    
    virtual std::shared_ptr<RType::Server::Client> getClient(uint32_t client_id) const = 0;
    virtual std::shared_ptr<RType::Server::Client> getClientByAddress(const std::shared_ptr<IAddress>& addr) const = 0;

protected:
    virtual void sendToClientRaw(uint32_t client_id, 
                                const void* data, 
                                size_t size) = 0;

    virtual void sendToAddressRaw(const std::shared_ptr<IAddress>& addr,
                                 const void* data, 
                                 size_t size) = 0;

    virtual void broadcastRaw(const void* data, 
                            size_t size, 
                            uint32_t exclude_id) = 0;
};

} // namespace RType::Network