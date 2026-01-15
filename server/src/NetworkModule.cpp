#include "NetworkModule.hpp"
#include <iostream>
#include <cstring>

namespace RType::Network {

NetworkModule::NetworkModule() 
    : _port(0), _running(false), _next_client_id(1)
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
        _socket.bind(port);
        _socket.setNonBlocking(true);
        _port = port;
        
        _running = true;
        _receive_thread = std::thread(&NetworkModule::receiveLoop, this);
        
        std::cout << "[NetworkModule] Started on port " << port << std::endl;
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
    
    _socket.close();
    
    std::cout << "[NetworkModule] Stopped" << std::endl;
}

void NetworkModule::receiveLoop()
{
    std::vector<uint8_t> buffer(MAX_PACKET_SIZE);
    
    while (_running)
    {
        Address from;
        ssize_t bytes = _socket.receiveFrom(buffer.data(), buffer.size(), from.addr);
        
        if (bytes < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        if (static_cast<size_t>(bytes) < sizeof(Protocol::PacketHeader)) {
            std::cerr << "[NetworkModule] Packet too small from " 
                      << from.toString() << std::endl;
            continue;
        }
        
        processRawPacket(buffer, bytes, from);
    }
}

void NetworkModule::processRawPacket(const std::vector<uint8_t>& buffer, 
                                      size_t size, 
                                      const Address& from)
{
    const auto& header = *reinterpret_cast<const Protocol::PacketHeader*>(buffer.data());
    
    if (!validatePacket(header, size)) {
        std::cerr << "[NetworkModule] Invalid packet from " 
                  << from.toString() << std::endl;
        return;
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
    
    size_t expected_size = sizeof(Protocol::PacketHeader) + header.payload_size;
    if (received_size != expected_size)
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

void NetworkModule::sendRawPacket(const void* data, size_t size, const Address& to)
{
    _socket.sendTo(data, size, to.addr);
}

void NetworkModule::sendToClientRaw(uint32_t client_id, const void* data, size_t size)
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    
    auto it = _clients_by_id.find(client_id);
    if (it == _clients_by_id.end())
        return;
    
    sendRawPacket(data, size, it->second->getAddress());
}

void NetworkModule::sendToAddressRaw(const Address& addr, const void* data, size_t size)
{
    sendRawPacket(data, size, addr);
}

void NetworkModule::broadcastRaw(const void* data, size_t size, uint32_t exclude_id)
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    
    for (const auto& [addr, client] : _clients_by_addr) {
        if (client->getId() != exclude_id && 
            client->getState() == Server::ClientState::CONNECTED) {
            sendRawPacket(data, size, addr);
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

void NetworkModule::disconnectClient(uint32_t client_id)
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    
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

uint32_t NetworkModule::registerClient(const Address& addr, 
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
        auto old_client = existing_id->second;
        _clients_by_addr.erase(old_client->getAddress());
        _clients_by_id.erase(id);
        std::cout << "[NetworkModule] Removed stale client with ID " << id << std::endl;
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

std::shared_ptr<Server::Client> NetworkModule::getClientByAddress(const Address& addr) const
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