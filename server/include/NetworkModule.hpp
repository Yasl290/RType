#pragma once
#include "INetworkModule.hpp"
#include "UDPSocket.hpp"
#include "Client.hpp"
#include "protocol/Protocol.hpp"
#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

namespace RType::Network {

class NetworkModule : public INetworkModule {
public:
    NetworkModule();
    ~NetworkModule() override;
    
    bool start(uint16_t port) override;
    void stop() override;
    bool isRunning() const override { return _running; }
    
    std::vector<ReceivedMessage> pollMessages() override;
    
    std::vector<uint32_t> getConnectedClients() const override;
    size_t getClientCount() const override;
    void disconnectClient(uint32_t client_id) override;
    
    bool hasAvailableSlot() const override;
    uint8_t findFreePlayerSlot() const override;
    
    uint32_t registerClient(const Address& addr, 
                            const std::string& name, 
                            uint8_t slot) override;
    void unregisterClient(uint32_t client_id) override;
    
    std::vector<uint32_t> checkTimeouts(std::chrono::seconds timeout) override;
    
    std::shared_ptr<Server::Client> getClient(uint32_t client_id) const;
    std::shared_ptr<Server::Client> getClientByAddress(const Address& addr) const;
    
protected:
    void sendToClientRaw(uint32_t client_id, 
                         const void* data, 
                         size_t size) override;
    void sendToAddressRaw(const Address& addr, 
                          const void* data, 
                          size_t size) override;
    void broadcastRaw(const void* data, 
                      size_t size, 
                      uint32_t exclude_id) override;
    
private:
    void receiveLoop();
    
    void processRawPacket(const std::vector<uint8_t>& buffer, 
                          size_t received_size, 
                          const Address& from);
    bool validatePacket(const Protocol::PacketHeader& header, size_t received_size);
    void sendRawPacket(const void* data, size_t size, const Address& to);
    
    UDPSocket _socket;
    uint16_t _port;
    
    std::atomic<bool> _running;
    std::thread _receive_thread;
    
    std::map<Address, std::shared_ptr<Server::Client>> _clients_by_addr;
    std::map<uint32_t, std::shared_ptr<Server::Client>> _clients_by_id;
    mutable std::mutex _clients_mutex;
    
    uint32_t _next_client_id;
    bool _player_slots[4];
    mutable std::mutex _slots_mutex;
    
    std::queue<ReceivedMessage> _message_queue;
    std::mutex _queue_mutex;
    
    static constexpr size_t MAX_PACKET_SIZE = 4096;
};

} // namespace RType::Network