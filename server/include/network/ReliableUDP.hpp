#pragma once

#include "network/ISocket.hpp"
#include <map>
#include <vector>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

namespace RType::Network {

class ReliableUDP {
public:
    using PacketCallback = std::function<void(const std::vector<uint8_t>&)>;
    
    explicit ReliableUDP(std::unique_ptr<ISocket> socket);
    ~ReliableUDP();
    
    void start();
    void stop();
    
    void sendReliable(const void* data, size_t size, const IAddress& dest);
    
    void sendUnreliable(const void* data, size_t size, const IAddress& dest);
    
    void setPacketCallback(PacketCallback callback);
    
private:
    struct PendingPacket {
        std::vector<uint8_t> data;
        std::shared_ptr<IAddress> dest;
        std::chrono::steady_clock::time_point sent_time;
        uint32_t sequence;
        uint8_t retries;
        bool acked;
    };
    
    struct ReliableHeader {
        uint32_t magic;
        uint32_t sequence;
        uint8_t is_ack;
        uint8_t padding[3];
    } __attribute__((packed));
    
    void receiveLoop();
    void retryLoop();
    void processReceivedPacket(const std::vector<uint8_t>& data,
                               const std::shared_ptr<IAddress>& from);
    void sendAck(uint32_t sequence, const IAddress& dest);
    void handleAck(uint32_t sequence);
    
    std::unique_ptr<ISocket> _socket;
    PacketCallback _callback;
    
    std::atomic<uint32_t> _nextSendSeq;
    
    uint32_t _nextRecvSeq;
    std::mutex _recvMutex;
    
    std::map<uint32_t, std::vector<uint8_t>> _recvBuffer;
    
    std::map<uint32_t, PendingPacket> _pendingAcks;
    std::mutex _pendingMutex;
    
    std::thread _receiveThread;
    std::thread _retryThread;
    std::atomic<bool> _running;
    
    static constexpr auto RETRY_TIMEOUT = std::chrono::milliseconds(100);
    static constexpr uint8_t MAX_RETRIES = 5;
    static constexpr uint32_t MAGIC = 0x52454C55;
};

} // namespace RType::Network