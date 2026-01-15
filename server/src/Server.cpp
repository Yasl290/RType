#include "Server.hpp"
#include <iostream>
#include <cstring>

namespace RType::Server {

Server::Server(uint16_t port)
    : _running(false)
    , _port(port)
    , _sequenceCounter(0)
    , _snapshotAccumulator(0.f)
    , _gameStarted(false)
    , _gameOver(false)
{
}

Server::~Server()
{
    stop();
}

void Server::start()
{
    if (_running) {
        return;
    }
    
    if (!_network.start(_port)) {
        std::cerr << "Failed to start network module" << std::endl;
        return;
    }
    
    _game.init();
    
    _running = true;
    _game_thread = std::thread(&Server::gameLoopThread, this);
    
    std::cout << "Server started on port " << _port << std::endl;
}

void Server::stop()
{
    if (!_running) {
        return;
    }
    
    _running = false;
    
    if (_game_thread.joinable()) {
        _game_thread.join();
    }
    
    _network.stop();
    
    std::cout << "Server stopped" << std::endl;
}

void Server::gameLoopThread()
{
    using clock = std::chrono::steady_clock;
    using namespace std::chrono;
    
    constexpr auto TICK_RATE = milliseconds(16);
    constexpr float FIXED_DT = 1.0f / 60.0f;
    constexpr float SNAPSHOT_DT = 1.0f / 20.0f;
    auto next_tick = clock::now();
    
    while (_running) {
        auto now = clock::now();
        
        if (now >= next_tick) {
            auto messages = _network.pollMessages();
            
            for (const auto& msg : messages) {
                handleMessage(msg);
            }

            if (_gameStarted && !_gameOver) {
                _game.update(FIXED_DT);
                
                // Broadcast des événements (spawn, destroy, fire, death)
                auto events = _game.pollEvents();
                for (const auto& event : events) {
                    broadcastEvent(event);
                }
                
                checkGameOver();
                
                _snapshotAccumulator += FIXED_DT;
                if (_snapshotAccumulator >= SNAPSHOT_DT) {
                    broadcastGameState();
                    broadcastScores();
                    _snapshotAccumulator = 0.f;
                }
            }
            
            checkTimeouts();
            
            next_tick += TICK_RATE;
        } else {
            std::this_thread::sleep_for(milliseconds(1));
        }
    }
}

void Server::handleMessage(const Network::ReceivedMessage& msg)
{
    auto type = static_cast<Protocol::PacketType>(msg.packet_type);
    
    switch (type) {
        case Protocol::PacketType::CONNECT_REQUEST:
            if (msg.payload_size >= sizeof(Protocol::ConnectRequest)) {
                handleConnectRequest(msg);
            }
            break;
            
        case Protocol::PacketType::DISCONNECT:
            if (msg.payload_size >= sizeof(Protocol::DisconnectPacket)) {
                handleDisconnect(msg);
            }
            break;
            
        case Protocol::PacketType::PLAYER_INPUT:
            if (msg.payload_size >= sizeof(Protocol::PlayerInput)) {
                handlePlayerInput(msg);
            }
            break;

        case Protocol::PacketType::READY_TO_PLAY:
            if (msg.payload_size >= sizeof(Protocol::ReadyToPlay)) {
                handleReadyToPlay(msg);
            }
            break;

        case Protocol::PacketType::GAME_ON:
            if (msg.payload_size >= sizeof(Protocol::GameOn)) {
                handleGameOn(msg);
            }
            break;
            
        case Protocol::PacketType::PING:
            handlePing(msg);
            break;
            
        default:
            std::cerr << "Unknown packet type: " 
                      << static_cast<int>(msg.packet_type) << std::endl;
    }
}

void Server::handleConnectRequest(const Network::ReceivedMessage& msg)
{
    Protocol::ConnectRequest request;
    std::memcpy(&request, msg.payload.data(), sizeof(Protocol::ConnectRequest));
    
    Protocol::ConnectResponse response;
    std::strncpy(response.server_version, SERVER_VERSION, 
                 sizeof(response.server_version) - 1);
    
    auto existing_client = _network.getClientByAddress(msg.source_addr);
    if (existing_client) {
        response.status = Protocol::ConnectionStatus::ACCEPTED;
        response.client_id = existing_client->getId();
        response.assigned_player_slot = existing_client->getPlayerSlot();
        
        _network.sendToAddress(msg.source_addr, response);
        return;
    }
    
    if (!_network.hasAvailableSlot()) {
        response.status = Protocol::ConnectionStatus::REJECTED_FULL;
        response.client_id = 0;
        
        _network.sendToAddress(msg.source_addr, response);
        std::cout << "Rejected " << msg.source_addr.toString() 
                  << " - Server full" << std::endl;
        return;
    }
    
    uint8_t slot = _network.findFreePlayerSlot();
    if (slot == 255) {
        response.status = Protocol::ConnectionStatus::REJECTED_FULL;
        response.client_id = 0;
        _network.sendToAddress(msg.source_addr, response);
        return;
    }
    
    std::string player_name(request.player_name);
    
    uint32_t new_id = _network.registerClient(msg.source_addr, player_name, slot);

    _readyByClient[new_id] = false;

    response.status = Protocol::ConnectionStatus::ACCEPTED;
    response.client_id = new_id;
    response.assigned_player_slot = slot;

    _network.sendToAddress(msg.source_addr, response);
    
    std::cout << "Client connected: " << player_name 
              << " [" << msg.source_addr.toString() << "] ID=" << new_id
              << " Slot=" << (int)slot << std::endl;

    float spawnX = 100.f;
    float spawnY = 150.f + (slot * 120.f);
    _game.spawnPlayer(new_id, spawnX, spawnY, slot);
    
    Protocol::DisconnectPacket notify;
    notify.client_id = new_id;
    notify.reason = 0;
    _network.broadcastExcept(new_id, notify);

    broadcastLobbyStatus();
}

void Server::handleDisconnect(const Network::ReceivedMessage& msg)
{
    Protocol::DisconnectPacket packet;
    std::memcpy(&packet, msg.payload.data(), sizeof(Protocol::DisconnectPacket));
    
    auto client = _network.getClient(packet.client_id);
    if (!client) {
        return;
    }
    
    std::cout << "Client disconnected: " << client->getName() 
              << " [" << client->getAddress().toString() << "]" << std::endl;
    
    _game.removePlayer(packet.client_id);
    _readyByClient.erase(packet.client_id);
    _network.disconnectClient(packet.client_id);

    if (_network.getClientCount() == 0 && (_gameStarted || _gameOver)) {
        _gameStarted = false;
        _gameOver = false;
        _game.init();
        std::cout << "[Server] All clients disconnected - resetting game" << std::endl;
    }

    _network.broadcastExcept(packet.client_id, packet);
    broadcastLobbyStatus();
}

void Server::handlePlayerInput(const Network::ReceivedMessage& msg)
{
    if (!_gameStarted) {
        return;
    }

    Protocol::PlayerInput input;
    std::memcpy(&input, msg.payload.data(), sizeof(Protocol::PlayerInput));

    _game.processInput(input.client_id, input.input_flags);
}

void Server::handleReadyToPlay(const Network::ReceivedMessage& msg)
{
    if (msg.payload_size < sizeof(Protocol::ReadyToPlay)) {
        return;
    }

    Protocol::ReadyToPlay packet;
    std::memcpy(&packet, msg.payload.data(), sizeof(Protocol::ReadyToPlay));

    auto client = _network.getClient(packet.client_id);
    if (!client) {
        return;
    }

    _readyByClient[packet.client_id] = (packet.ready != 0);
    broadcastLobbyStatus();

    if (!_gameStarted && !_gameOver && areAllPlayersReady()) {
        _gameStarted = true;

        Protocol::GameOn start;
        start.client_id = packet.client_id;
        _network.broadcast(start);

        broadcastLobbyStatus();
    }
}

void Server::handleGameOn(const Network::ReceivedMessage& msg)
{
    Protocol::GameOn packet;
    std::memcpy(&packet, msg.payload.data(), sizeof(Protocol::GameOn));

    _gameStarted = true;
    _network.broadcast(packet);
    broadcastLobbyStatus();
}

void Server::handlePing(const Network::ReceivedMessage& msg)
{
    Protocol::PacketHeader header;
    std::memcpy(&header, msg.payload.data(), sizeof(Protocol::PacketHeader));
    
    Protocol::PacketHeader pong;
    pong.type = Protocol::PacketType::PONG;
    pong.sequence_number = header.sequence_number;
    
    _network.sendToClient(msg.client_id, pong);
}

void Server::checkTimeouts()
{
    auto timed_out = _network.checkTimeouts(CLIENT_TIMEOUT);
    
    if (timed_out.empty()) {
        return;
    }
    
    for (uint32_t id : timed_out) {
        auto client = _network.getClient(id);
        if (client) {
            std::cout << "Client timed out: " << client->getName() << std::endl;
            
            Protocol::DisconnectPacket notify;
            notify.client_id = id;
            notify.reason = 1;
            
            _network.broadcast(notify);
            _game.removePlayer(id);
            _readyByClient.erase(id);
            _network.disconnectClient(id);
        }
    }

    if (_network.getClientCount() == 0 && (_gameStarted || _gameOver)) {
        _gameStarted = false;
        _gameOver = false;
        _game.init();
        std::cout << "[Server] All clients timed out - resetting game" << std::endl;
    }

    broadcastLobbyStatus();
}

bool Server::areAllPlayersReady() const
{
    auto connected = _network.getConnectedClients();
    if (connected.empty()) {
        return false;
    }

    for (uint32_t id : connected) {
        auto it = _readyByClient.find(id);
        if (it == _readyByClient.end() || !it->second) {
            return false;
        }
    }

    return true;
}

void Server::broadcastLobbyStatus()
{
    Protocol::LobbyStatus status;
    status.max_players = 4;

    auto connected = _network.getConnectedClients();
    status.players_connected = static_cast<uint8_t>(connected.size());
    status.players_ready = 0;
    status.ready_mask = 0;
    status.game_started = _gameStarted ? 1 : 0;

    for (uint32_t id : connected) {
        auto client = _network.getClient(id);
        if (!client) {
            continue;
        }

        uint8_t slot = client->getPlayerSlot();
        bool ready = false;
        auto it = _readyByClient.find(id);
        if (it != _readyByClient.end()) {
            ready = it->second;
        }

        if (ready) {
            status.players_ready++;
            if (slot < 8) {
                status.ready_mask |= static_cast<uint8_t>(1u << slot);
            }
        }
    }

    _network.broadcast(status);
}

void Server::broadcastGameState()
{
    if (_network.getClientCount() == 0) {
        return;
    }

    auto snapshots = _game.getWorldSnapshot();
    for (const auto& snap : snapshots) {
        Protocol::EntityUpdate packet;
        packet.header.sequence_number = _sequenceCounter++;
        packet.entity_id = snap.entity_id;
        packet.pos_x = snap.pos_x;
        packet.pos_y = snap.pos_y;
        packet.vel_x = snap.vel_x;
        packet.vel_y = snap.vel_y;
        packet.entity_type = snap.entity_type;
        packet.hp_current = snap.hp_current;
        packet.hp_max = snap.hp_max;
        packet.player_slot = snap.player_slot;

        _network.broadcast(packet);
    }
}

void Server::broadcastScores()
{
    auto clients = _network.getConnectedClients();
    
    for (uint32_t client_id : clients) {
        Protocol::ScoreUpdate packet;
        packet.client_id = client_id;
        packet.score = _game.getPlayerScore(client_id);
        packet.enemies_killed = _game.getPlayerKills(client_id);
        
        _network.broadcast(packet);
    }
}

void Server::checkGameOver()
{
    if (_game.areAllPlayersDead()) {
        _gameOver = true;
        _gameStarted = false;
        broadcastGameOver();
        
        std::cout << "[Server] Game Over! All players dead." << std::endl;
    }
}

void Server::broadcastGameOver()
{
    Protocol::GameOver packet;
    
    auto finalScores = _game.getFinalScores();
    packet.num_players = static_cast<uint8_t>(std::min(finalScores.size(), size_t(4)));
    
    for (size_t i = 0; i < packet.num_players; ++i) {
        packet.players[i] = finalScores[i];
    }
    
    _network.broadcast(packet);
    
    std::cout << "[Server] Broadcasted Game Over with " 
              << (int)packet.num_players << " player scores" << std::endl;
}

void Server::broadcastEvent(const GameEvent& event)
{
    switch (event.type) {
        case EventType::ENTITY_SPAWNED: {
            Protocol::EntitySpawn packet;
            packet.entity_id = event.entity_id;
            packet.entity_type = event.entity_type;
            packet.pos_x = event.pos_x;
            packet.pos_y = event.pos_y;
            _network.broadcast(packet);
            
            std::cout << "[Server] Entity spawned: ID=" << event.entity_id 
                      << " Type=" << (int)event.entity_type 
                      << " Pos=(" << event.pos_x << "," << event.pos_y << ")" << std::endl;
            break;
        }
        
        case EventType::ENTITY_DESTROYED: {
            Protocol::EntityDestroy packet;
            packet.entity_id = event.entity_id;
            packet.killer_id = event.related_id;
            packet.reason = event.extra_data;
            _network.broadcast(packet);
            
            std::cout << "[Server] Entity destroyed: ID=" << event.entity_id 
                      << " Killer=" << event.related_id << std::endl;
            break;
        }
        
        case EventType::ENTITY_FIRED: {
            Protocol::EntityFire packet;
            packet.shooter_id = event.related_id;
            packet.projectile_id = event.entity_id;
            packet.pos_x = event.pos_x;
            packet.pos_y = event.pos_y;
            packet.projectile_type = event.extra_data;
            _network.broadcast(packet);
            
            std::cout << "[Server] Entity fired: Shooter=" << event.related_id 
                      << " Projectile=" << event.entity_id 
                      << " Type=" << (int)event.extra_data << std::endl;
            break;
        }
        
        case EventType::PLAYER_DIED: {
            Protocol::PlayerDeath packet;
            packet.player_id = event.entity_id;
            packet.killer_id = event.related_id;
            packet.death_type = event.extra_data;
            _network.broadcast(packet);
            
            std::cout << "[Server] Player died: ID=" << event.entity_id 
                      << " Killer=" << event.related_id 
                      << " Type=" << (int)event.extra_data << std::endl;
            break;
        }
    }
}

} // namespace RType::Server