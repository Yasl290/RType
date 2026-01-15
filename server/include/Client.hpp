#pragma once

#include "UDPSocket.hpp"
#include <chrono>
#include <string>

namespace RType::Server {

enum class ClientState {
    CONNECTING,
    CONNECTED,
    DISCONNECTED
};

class Client {
public:
    Client(uint32_t id, const Network::Address& addr, const std::string& name)
        : _id(id), _address(addr), _name(name), _state(ClientState::CONNECTING),
          _player_slot(0), _sequence_number(0) {
        updateLastActivity();
    }
    
    uint32_t getId() const { return _id; }
    const Network::Address& getAddress() const { return _address; }
    const std::string& getName() const { return _name; }
    ClientState getState() const { return _state; }
    uint8_t getPlayerSlot() const { return _player_slot; }
    uint32_t getSequenceNumber() const { return _sequence_number; }
    
    void setState(ClientState state) { _state = state; }
    void setPlayerSlot(uint8_t slot) { _player_slot = slot; }
    void incrementSequence() { _sequence_number++; }
    
    void updateLastActivity() {
        _last_activity = std::chrono::steady_clock::now();
    }
    
    bool isTimedOut(std::chrono::seconds timeout) const
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(
            now - _last_activity) > timeout;
    }
    
    auto getLastActivity() const { return _last_activity; }

private:
    uint32_t _id;
    Network::Address _address;
    std::string _name;
    ClientState _state;
    uint8_t _player_slot;
    uint32_t _sequence_number;
    std::chrono::steady_clock::time_point _last_activity;
};

} // namespace RType::Server

