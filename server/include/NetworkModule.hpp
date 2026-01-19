#pragma once

#include "INetworkModule.hpp"
#include "network/ISocket.hpp"
#include "network/UDPAddress.hpp"
#include "Client.hpp"
#include "protocol/Protocol.hpp"
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <chrono>

namespace RType::Network {

struct PendingReliablePacket {
    std::vector<uint8_t> data;
    std::shared_ptr<IAddress> dest;
    std::chrono::steady_clock::time_point last_sent;
    uint16_t sequence;
    uint8_t retries;
};

class NetworkModule : public INetworkModule {
public:
    explicit NetworkModule(std::unique_ptr<ISocket> socket);
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

    uint32_t registerClient(const std::shared_ptr<IAddress>& addr,
                           const std::string& name,
                           uint8_t slot) override;

    void unregisterClient(uint32_t client_id) override;

    std::vector<uint32_t> checkTimeouts(std::chrono::seconds timeout) override;

    std::shared_ptr<Server::Client> getClient(uint32_t client_id) const override;
    std::shared_ptr<Server::Client> getClientByAddress(const std::shared_ptr<IAddress>& addr) const override;

protected:
    void sendToClientRaw(uint32_t client_id, const void* data, size_t size) override;
    void sendToAddressRaw(const std::shared_ptr<IAddress>& addr, const void* data, size_t size) override;
    void broadcastRaw(const void* data, size_t size, uint32_t exclude_id) override;

private:
    void receiveLoop();
    void retryLoop();
    void processRawPacket(const std::vector<uint8_t>& buffer,
                         size_t received_size,
                         const std::shared_ptr<IAddress>& from);
    bool validatePacket(const Protocol::PacketHeader& header, size_t received_size);
    void sendRawPacket(const void* data, size_t size, const IAddress& to, bool reliable);
    void handleAck(uint16_t sequence, const std::shared_ptr<IAddress>& from);
    void sendAck(uint16_t sequence, const IAddress& dest);
    
    void disconnectClientUnsafe(uint32_t client_id);

    std::unique_ptr<ISocket> _socket;
    uint16_t _port;

    std::atomic<bool> _running;
    std::thread _receive_thread;
    std::thread _retry_thread;

    std::unordered_map<std::shared_ptr<IAddress>,
                       std::shared_ptr<Server::Client>,
                       AddressHash,
                       AddressEqual> _clients_by_addr;

    std::map<uint32_t, std::shared_ptr<Server::Client>> _clients_by_id;
    mutable std::mutex _clients_mutex;

    uint32_t _next_client_id;

    bool _player_slots[4];
    mutable std::mutex _slots_mutex;

    std::queue<ReceivedMessage> _message_queue;
    std::mutex _queue_mutex;
    
    std::atomic<uint16_t> _next_sequence;
    std::map<uint16_t, PendingReliablePacket> _pending_reliable;
    std::mutex _reliable_mutex;
    
    std::unordered_map<std::shared_ptr<IAddress>, std::unordered_set<uint16_t>, AddressHash, AddressEqual> _received_sequences;
    std::mutex _dedup_mutex;

    static constexpr size_t MAX_PACKET_SIZE = 4096;
    static constexpr auto RETRY_TIMEOUT = std::chrono::milliseconds(100);
    static constexpr uint8_t MAX_RETRIES = 5;
    static constexpr size_t MAX_SEQUENCE_HISTORY = 128;
};

} // namespace RType::Network