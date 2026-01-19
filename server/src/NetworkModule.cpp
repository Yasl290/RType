#include "NetworkModule.hpp"
#include <iostream>
#include <cstring>

namespace RType::Network {

NetworkModule::NetworkModule(std::unique_ptr<ISocket> socket)
    : _socket(std::move(socket))
    , _port(0)
    , _running(false)
    , _next_client_id(1)
    , _next_sequence(1)
{
    std::memset(_player_slots, 0, sizeof(_player_slots));
}

NetworkModule::~NetworkModule()
{
    stop();
}

bool NetworkModule::start(uint16_t port)
{
    if (_running)
        return false;
    
    try {
        _socket->bind(port);
        _socket->setNonBlocking(true);
        _port = port;
        
        _running = true;
        _receive_thread = std::thread(&NetworkModule::receiveLoop, this);
        _retry_thread = std::thread(&NetworkModule::retryLoop, this);
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[NetworkModule] Failed to start: " << e.what() << std::endl;
        return false;
    }
}

void NetworkModule::stop()
{
    if (!_running)
        return;
    
    _running = false;
    
    if (_receive_thread.joinable())
        _receive_thread.join();
    
    if (_retry_thread.joinable())
        _retry_thread.join();
    
    _socket->close();
}

void NetworkModule::receiveLoop()
{
    std::vector<uint8_t> buffer(MAX_PACKET_SIZE);
    
    while (_running)
    {
        UDPAddress from;
        ssize_t bytes = _socket->receiveFrom(buffer.data(), buffer.size(), from);
        
        if (bytes < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        if (static_cast<size_t>(bytes) < sizeof(Protocol::PacketHeader)) {
            continue;
        }
        
        std::vector<uint8_t> packet(buffer.begin(), buffer.begin() + bytes);
        auto addr = std::make_shared<UDPAddress>(from);
        processRawPacket(packet, bytes, addr);
    }
}

void NetworkModule::retryLoop()
{
    while (_running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        auto now = std::chrono::steady_clock::now();
        
        std::lock_guard<std::mutex> lock(_reliable_mutex);
        
        for (auto it = _pending_reliable.begin(); it != _pending_reliable.end(); )
        {
            auto& pending = it->second;
            
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - pending.last_sent
            );
            
            if (elapsed >= RETRY_TIMEOUT) {
                if (pending.retries >= MAX_RETRIES) {
                    it = _pending_reliable.erase(it);
                    continue;
                }
                
                _socket->sendTo(pending.data.data(), pending.data.size(), *pending.dest);
                pending.last_sent = now;
                pending.retries++;
            }
            
            ++it;
        }
    }
}

void NetworkModule::processRawPacket(const std::vector<uint8_t>& buffer, 
                                      size_t size, 
                                      const std::shared_ptr<IAddress>& from)
{
    const auto& header = *reinterpret_cast<const Protocol::PacketHeader*>(buffer.data());
    
    if (!validatePacket(header, size)) {
        return;
    }
    
    if (header.type == Protocol::PacketType::ACK) {
        if (size >= sizeof(Protocol::AckPacket)) {
            const auto& ack = *reinterpret_cast<const Protocol::AckPacket*>(buffer.data());
            handleAck(ack.ack_sequence, from);
        }
        return;
    }
    
    if (header.isReliable()) {
        {
            std::lock_guard<std::mutex> lock(_dedup_mutex);
            
            auto& sequences = _received_sequences[from];
            
            if (sequences.count(header.sequence_number)) {
                sendAck(header.sequence_number, *from);
                return;
            }
            
            sequences.insert(header.sequence_number);
            
            if (sequences.size() > MAX_SEQUENCE_HISTORY) {
                auto oldest = *sequences.begin();
                sequences.erase(oldest);
            }
        }
        
        sendAck(header.sequence_number, *from);
    }
    
    {
        std::lock_guard<std::mutex> lock(_clients_mutex);
        auto it = _clients_by_addr.find(from);
        if (it != _clients_by_addr.end()) {
            it->second->updateLastActivity();
        }
    }
    
    ReceivedMessage msg;
    msg.packet_type = static_cast<uint8_t>(header.type);
    msg.payload_size = size;
    msg.payload.assign(buffer.begin(), buffer.begin() + size);
    msg.source_addr = from;
    
    {
        std::lock_guard<std::mutex> lock(_clients_mutex);
        auto it = _clients_by_addr.find(from);
        if (it != _clients_by_addr.end()) {
            msg.client_id = it->second->getId();
        } else {
            msg.client_id = 0;
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        _message_queue.push(std::move(msg));
    }
}

bool NetworkModule::validatePacket(const Protocol::PacketHeader& header, 
                                    size_t received_size)
{
    if (!Protocol::IsValidPacket(header))
        return false;
    
    if (received_size < sizeof(Protocol::PacketHeader))
        return false;
    
    return true;
}

std::vector<ReceivedMessage> NetworkModule::pollMessages()
{
    std::lock_guard<std::mutex> lock(_queue_mutex);
    
    std::vector<ReceivedMessage> messages;
    
    while (!_message_queue.empty()) {
        messages.push_back(std::move(_message_queue.front()));
        _message_queue.pop();
    }
    
    return messages;
}

void NetworkModule::sendRawPacket(const void* data, size_t size, const IAddress& to, bool reliable)
{
    if (reliable) {
        const auto& header = *reinterpret_cast<const Protocol::PacketHeader*>(data);
        
        std::vector<uint8_t> packet(static_cast<const uint8_t*>(data),
                                     static_cast<const uint8_t*>(data) + size);
        
        PendingReliablePacket pending;
        pending.data = std::move(packet);
        pending.dest = to.clone();
        pending.last_sent = std::chrono::steady_clock::now();
        pending.sequence = header.sequence_number;
        pending.retries = 0;
        
        {
            std::lock_guard<std::mutex> lock(_reliable_mutex);
            _pending_reliable[header.sequence_number] = std::move(pending);
        }
    }
    
    _socket->sendTo(data, size, to);
}

void NetworkModule::sendAck(uint16_t sequence, const IAddress& dest)
{
    Protocol::AckPacket ack;
    ack.header.sequence_number = 0;
    ack.ack_sequence = sequence;
    
    _socket->sendTo(&ack, sizeof(ack), dest);
}

void NetworkModule::handleAck(uint16_t sequence, const std::shared_ptr<IAddress>&)
{
    std::lock_guard<std::mutex> lock(_reliable_mutex);
    _pending_reliable.erase(sequence);
}

void NetworkModule::sendToClientRaw(uint32_t client_id, const void* data, size_t size)
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    
    auto it = _clients_by_id.find(client_id);
    if (it == _clients_by_id.end())
        return;
    
    const auto& header = *reinterpret_cast<const Protocol::PacketHeader*>(data);
    sendRawPacket(data, size, *it->second->getAddress(), header.isReliable());
}

void NetworkModule::sendToAddressRaw(const std::shared_ptr<IAddress>& addr, 
                                      const void* data, 
                                      size_t size)
{
    const auto& header = *reinterpret_cast<const Protocol::PacketHeader*>(data);
    sendRawPacket(data, size, *addr, header.isReliable());
}

void NetworkModule::broadcastRaw(const void* data, size_t size, uint32_t exclude_id)
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    
    const auto& header = *reinterpret_cast<const Protocol::PacketHeader*>(data);
    bool reliable = header.isReliable();
    
    for (const auto& [addr, client] : _clients_by_addr) {
        if (client->getId() != exclude_id && 
            client->getState() == Server::ClientState::CONNECTED) {
            sendRawPacket(data, size, *addr, reliable);
        }
    }
}

std::vector<uint32_t> NetworkModule::getConnectedClients() const
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    
    std::vector<uint32_t> clients;
    for (const auto& [id, client] : _clients_by_id) {
        if (client->getState() == Server::ClientState::CONNECTED)
            clients.push_back(id);
    }
    return clients;
}

size_t NetworkModule::getClientCount() const
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    return _clients_by_id.size();
}

void NetworkModule::disconnectClientUnsafe(uint32_t client_id)
{
    auto it = _clients_by_id.find(client_id);
    if (it == _clients_by_id.end())
        return;
    
    auto client = it->second;
    
    {
        std::lock_guard<std::mutex> slot_lock(_slots_mutex);
        _player_slots[client->getPlayerSlot()] = false;
    }
    
    _clients_by_addr.erase(client->getAddress());
    _clients_by_id.erase(client_id);
}

void NetworkModule::disconnectClient(uint32_t client_id)
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    disconnectClientUnsafe(client_id);
}

uint32_t NetworkModule::registerClient(const std::shared_ptr<IAddress>& addr,
                                        const std::string& name,
                                        uint8_t slot)
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    
    auto it = _clients_by_addr.find(addr);
    if (it != _clients_by_addr.end())
        return it->second->getId();

    uint32_t id = slot + 1;
    
    auto existing_id = _clients_by_id.find(id);
    if (existing_id != _clients_by_id.end()) {
        disconnectClientUnsafe(id);
    }
    
    auto client = std::make_shared<Server::Client>(id, addr, name);
    client->setPlayerSlot(slot);
    client->setState(Server::ClientState::CONNECTED);
    
    _clients_by_addr[addr] = client;
    _clients_by_id[id] = client;
    
    {
        std::lock_guard<std::mutex> slot_lock(_slots_mutex);
        _player_slots[slot] = true;
    }
    
    return id;
}

void NetworkModule::unregisterClient(uint32_t client_id)
{
    disconnectClient(client_id);
}

std::shared_ptr<Server::Client> NetworkModule::getClient(uint32_t client_id) const
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    
    auto it = _clients_by_id.find(client_id);
    if (it != _clients_by_id.end())
        return it->second;
    return nullptr;
}

std::shared_ptr<Server::Client> NetworkModule::getClientByAddress(const std::shared_ptr<IAddress>& addr) const
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    
    auto it = _clients_by_addr.find(addr);
    if (it != _clients_by_addr.end())
        return it->second;
    return nullptr;
}

bool NetworkModule::hasAvailableSlot() const
{
    std::lock_guard<std::mutex> lock(_slots_mutex);
    for (int i = 0; i < 4; i++) {
        if (!_player_slots[i])
            return true;
    }
    return false;
}

uint8_t NetworkModule::findFreePlayerSlot() const
{
    std::lock_guard<std::mutex> lock(_slots_mutex);
    for (uint8_t i = 0; i < 4; i++) {
        if (!_player_slots[i])
            return i;
    }
    return 255;
}

std::vector<uint32_t> NetworkModule::checkTimeouts(std::chrono::seconds timeout)
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    
    std::vector<uint32_t> timed_out;
    
    for (const auto& [id, client] : _clients_by_id) {
        if (client->isTimedOut(timeout)) {
            timed_out.push_back(id);
        }
    }
    
    return timed_out;
}

} // namespace RType::Network