#include "Server.hpp"
#include "NetworkModule.hpp"
#include "network/UDPSocket.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <unordered_set>
#include <cmath>

namespace RType::Server {

Server::Server(uint16_t port)
    : _running(false)
    , _port(port)
    , _sequenceCounter(1)
    , _snapshotAccumulator(0.f)
    , _gameStarted(false)
    , _gameOver(false)
{
    auto socket = std::make_unique<RType::Network::UDPSocket>();
    _network = std::make_unique<RType::Network::NetworkModule>(std::move(socket));
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
    
    if (!_network->start(_port)) {
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
    
    _network->stop();
    
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
            auto messages = _network->pollMessages();
            
            for (const auto& msg : messages) {
                handleMessage(msg);
            }

            if (_gameStarted && !_gameOver) {
                _game.update(FIXED_DT);
                auto events = _game.pollEvents();
                for (const auto& event : events) {
                    broadcastEvent(event);
                }

                auto networkEvents = _game.pollNetworkEvents();
                for (const auto& netEvent : networkEvents) {
                    broadcastNetworkEvent(netEvent);
                    
                    if (netEvent.event_type == RType::Protocol::GameEventType::GAME_OVER) {
                        std::cout << "[Server] GAME_OVER event detected, triggering final game over..." << std::endl;
                        _gameOver = true;
                        _gameStarted = false;
                        broadcastGameOver();
                    }
                }
                
                if (_game.isGameOver() && !_gameOver) {
                    std::cout << "[Server] GameModule reports game over! Broadcasting..." << std::endl;
                    _gameOver = true;
                    _gameStarted = false;
                    broadcastGameOver();
                }
                
                if (_game.areAllPlayersDead() && !_gameOver) {
                    std::cout << "[Server] All players dead! Broadcasting game over..." << std::endl;
                    _gameOver = true;
                    _gameStarted = false;
                    broadcastGameOver();
                }
                
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
            break;
    }
}

void Server::handleConnectRequest(const Network::ReceivedMessage& msg)
{
    Protocol::ConnectRequest request;
    std::memcpy(&request, msg.payload.data(), sizeof(Protocol::ConnectRequest));
    
    Protocol::ConnectResponse response;
    response.header.sequence_number = _sequenceCounter++;
    std::strncpy(response.server_version, SERVER_VERSION, 
                 sizeof(response.server_version) - 1);
    
    auto existing_client = _network->getClientByAddress(msg.source_addr);
    if (existing_client) {
        response.status = Protocol::ConnectionStatus::ACCEPTED;
        response.client_id = existing_client->getId();
        response.assigned_player_slot = existing_client->getPlayerSlot();
        
        _network->sendToAddress(msg.source_addr, response);
        return;
    }
    
    if (!_network->hasAvailableSlot()) {
        response.status = Protocol::ConnectionStatus::REJECTED_FULL;
        response.client_id = 0;
        
        _network->sendToAddress(msg.source_addr, response);
        return;
    }
    
    uint8_t slot = _network->findFreePlayerSlot();
    if (slot == 255) {
        response.status = Protocol::ConnectionStatus::REJECTED_FULL;
        response.client_id = 0;
        _network->sendToAddress(msg.source_addr, response);
        return;
    }
    
    std::string player_name(request.player_name);
    
    uint32_t new_id = _network->registerClient(msg.source_addr, player_name, slot);

    _readyByClient[new_id] = false;

    response.status = Protocol::ConnectionStatus::ACCEPTED;
    response.client_id = new_id;
    response.assigned_player_slot = slot;

    _network->sendToAddress(msg.source_addr, response);
    
    std::cout << "Client connected: " << player_name 
              << " [" << msg.source_addr->toString() << "] ID=" << new_id
              << " Slot=" << (int)slot << std::endl;

    float spawnX = 100.f;
    float spawnY = 150.f + (slot * 120.f);
    _game.spawnPlayer(new_id, spawnX, spawnY, slot);
    
    Protocol::DisconnectPacket notify;
    notify.header.sequence_number = _sequenceCounter++;
    notify.client_id = new_id;
    notify.reason = 0;
    _network->broadcastExcept(new_id, notify);

    broadcastLobbyStatus();
}

void Server::handleDisconnect(const Network::ReceivedMessage& msg)
{
    Protocol::DisconnectPacket packet;
    std::memcpy(&packet, msg.payload.data(), sizeof(Protocol::DisconnectPacket));
    
    auto client = _network->getClient(packet.client_id);
    if (!client) {
        return;
    }
    
    std::cout << "Client disconnected: " << client->getName() 
              << " [" << client->getAddress()->toString() << "]" << std::endl;
    
    _game.removePlayer(packet.client_id);
    _readyByClient.erase(packet.client_id);
    _network->disconnectClient(packet.client_id);

    if (_network->getClientCount() == 0 && (_gameStarted || _gameOver)) {
        _gameStarted = false;
        _gameOver = false;
        _game.init();
    }

    packet.header.sequence_number = _sequenceCounter++;
    _network->broadcastExcept(packet.client_id, packet);
    broadcastLobbyStatus();
}

void Server::handlePlayerInput(const Network::ReceivedMessage& msg)
{
    if (!_gameStarted) {
        return;
    }

    Protocol::PlayerInput input;
    std::memcpy(&input, msg.payload.data(), sizeof(Protocol::PlayerInput));

    // ============================================================================
    // RATE LIMITING - Track #2
    // Prevents input spam (max 60 inputs/sec per client)
    // Protects against malicious clients flooding the server
    // ============================================================================
    {
        std::lock_guard<std::mutex> lock(_inputTimeMutex);
        auto now = std::chrono::steady_clock::now();
        auto& lastTime = _lastInputTime[input.client_id];

        // Check if enough time has passed since last input (16ms = ~60 FPS)
        if (lastTime.time_since_epoch().count() > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime);
            if (elapsed < MIN_INPUT_INTERVAL) {
                // Drop input if too frequent (anti-spam protection)
                return;
            }
        }

        lastTime = now;
    }

    _game.processInput(input.client_id, input.input_flags);
}

void Server::handleReadyToPlay(const Network::ReceivedMessage& msg)
{
    if (msg.payload_size < sizeof(Protocol::ReadyToPlay)) {
        return;
    }

    Protocol::ReadyToPlay packet;
    std::memcpy(&packet, msg.payload.data(), sizeof(Protocol::ReadyToPlay));

    auto client = _network->getClient(packet.client_id);
    if (!client) {
        return;
    }

    _readyByClient[packet.client_id] = (packet.ready != 0);
    broadcastLobbyStatus();

    if (!_gameStarted && !_gameOver && areAllPlayersReady()) {
        _gameStarted = true;

        Protocol::GameOn start;
        start.header.sequence_number = _sequenceCounter++;
        start.client_id = packet.client_id;
        _network->broadcast(start);

        broadcastLobbyStatus();
    }
}

void Server::handleGameOn(const Network::ReceivedMessage& msg)
{
    Protocol::GameOn packet;
    std::memcpy(&packet, msg.payload.data(), sizeof(Protocol::GameOn));

    _gameStarted = true;
    packet.header.sequence_number = _sequenceCounter++;
    _network->broadcast(packet);
    broadcastLobbyStatus();
}

void Server::handlePing(const Network::ReceivedMessage& msg)
{
    Protocol::PacketHeader header;
    std::memcpy(&header, msg.payload.data(), sizeof(Protocol::PacketHeader));
    
    Protocol::PacketHeader pong;
    pong.type = Protocol::PacketType::PONG;
    pong.sequence_number = header.sequence_number;
    
    _network->sendToClient(msg.client_id, pong);
}

void Server::checkTimeouts()
{
    auto timed_out = _network->checkTimeouts(CLIENT_TIMEOUT);
    
    if (timed_out.empty()) {
        return;
    }
    
    for (uint32_t id : timed_out) {
        auto client = _network->getClient(id);
        if (client) {
            Protocol::DisconnectPacket notify;
            notify.header.sequence_number = _sequenceCounter++;
            notify.client_id = id;
            notify.reason = 1;
            
            _network->broadcast(notify);
            _game.removePlayer(id);
            _readyByClient.erase(id);
            _network->disconnectClient(id);
        }
    }

    if (_network->getClientCount() == 0 && (_gameStarted || _gameOver)) {
        _gameStarted = false;
        _gameOver = false;
        _game.init();
    }

    broadcastLobbyStatus();
}

bool Server::areAllPlayersReady() const
{
    auto connected = _network->getConnectedClients();
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
    status.header.sequence_number = _sequenceCounter++;
    status.max_players = 4;

    auto connected = _network->getConnectedClients();
    status.players_connected = static_cast<uint8_t>(connected.size());
    status.players_ready = 0;
    status.ready_mask = 0;
    status.game_started = _gameStarted ? 1 : 0;

    for (uint32_t id : connected) {
        auto client = _network->getClient(id);
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

    _network->broadcast(status);
}

void Server::broadcastGameState()
{
    if (_network->getClientCount() == 0) {
        return;
    }

    auto snapshots = _game.getWorldSnapshot();
    auto clients = _network->getConnectedClients();

    // ============================================================================
    // ADVANCED NETWORKING - Track #2
    // Combines: Delta Compression + Quantization + Packet Batching
    // Bandwidth reduction: ~80% total
    // ============================================================================

    for (uint32_t client_id : clients) {
        std::lock_guard<std::mutex> lock(_snapshotMutex);

        // Get last known state for this client
        auto& lastSnapshots = _lastSnapshotsByClient[client_id];

        // Build batched update with quantized data
        Protocol::BatchedEntityUpdate batch;
        batch.header.sequence_number = _sequenceCounter++;
        batch.entity_count = 0;

        for (const auto& snap : snapshots) {
            // Check if we have a previous snapshot for delta compression
            auto it = lastSnapshots.find(snap.entity_id);
            bool hasChanged = false;

            // CRITICAL: Always send players (type=0 or player_slot != 255) and bosses (type=5)
            // Players are only 4 entities max, and must be visible at all times
            // Bosses stop moving when reaching target position, causing delta compression to skip them
            // Delta compression only for regular enemies/projectiles (80% of entities)
            bool isPlayer = (snap.entity_type == 0 || snap.player_slot != 255);
            bool isBoss = (snap.entity_type == 5);

            if (it == lastSnapshots.end()) {
                // New entity - send full state
                hasChanged = true;
            } else if (isPlayer || isBoss) {
                // Always send players and bosses to prevent invisible entity bug
                hasChanged = true;
            } else {
                // Delta compression: Only send if something changed
                const auto& last = it->second;

                // Tolerance for float comparison (0.5 units for position, 1.0 for HP)
                constexpr float POS_EPSILON = 0.5f;
                constexpr float HP_EPSILON = 1.0f;

                bool posChanged = (std::abs(snap.pos_x - last.pos_x) > POS_EPSILON ||
                                   std::abs(snap.pos_y - last.pos_y) > POS_EPSILON);
                bool velChanged = (snap.vel_x != last.vel_x || snap.vel_y != last.vel_y);
                bool hpChanged = (std::abs(snap.hp_current - last.hp_current) > HP_EPSILON ||
                                  std::abs(snap.hp_max - last.hp_max) > HP_EPSILON);
                bool typeChanged = (snap.entity_type != last.entity_type);
                bool slotChanged = (snap.player_slot != last.player_slot);

                hasChanged = posChanged || velChanged || hpChanged || typeChanged || slotChanged;
            }

            if (hasChanged) {
                // Quantization: Convert float32 to int16 (saves ~50% on numeric data)
                Protocol::QuantizedEntityData& qdata = batch.entities[batch.entity_count];
                qdata.entity_id = snap.entity_id;
                qdata.pos_x = Protocol::quantizePosition(snap.pos_x);
                qdata.pos_y = Protocol::quantizePosition(snap.pos_y);
                qdata.vel_x = Protocol::quantizeVelocity(snap.vel_x);
                qdata.vel_y = Protocol::quantizeVelocity(snap.vel_y);
                qdata.hp_current = Protocol::quantizeHP(snap.hp_current);
                qdata.hp_max = Protocol::quantizeHP(snap.hp_max);
                qdata.entity_type = snap.entity_type;
                qdata.player_slot = snap.player_slot;

                batch.entity_count++;

                // Update last snapshot for this entity
                lastSnapshots[snap.entity_id] = snap;

                // Send batch if full (64 entities max per packet)
                if (batch.entity_count >= 64) {
                    _network->sendToClient(client_id, batch);
                    batch.entity_count = 0;
                    batch.header.sequence_number = _sequenceCounter++;
                }
            }
        }

        // Send remaining entities in batch
        if (batch.entity_count > 0) {
            _network->sendToClient(client_id, batch);
        }

        // Clean up despawned entities from last snapshots
        std::unordered_set<uint32_t> currentEntityIds;
        for (const auto& snap : snapshots) {
            currentEntityIds.insert(snap.entity_id);
        }

        for (auto it = lastSnapshots.begin(); it != lastSnapshots.end();) {
            if (currentEntityIds.find(it->first) == currentEntityIds.end()) {
                it = lastSnapshots.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Bandwidth Analysis (before vs after):
    // BEFORE: 50 entities × 36 bytes × 20 Hz = 36 KB/s per client
    // AFTER:  ~15 entities × 20 bytes × 20 Hz = 6.0 KB/s per client (83% reduction!)
    //
    // Features breakdown:
    // - Delta Compression: Only send changed entities (~80% reduction for enemies)
    //   Players (4 max) and bosses (1 max) always sent to prevent invisible entity bug
    // - Quantization: float32 → int16 (~50% reduction on numeric fields)
    // - Batching: 50 packets → 1-2 packets (~98% less UDP overhead)
}

void Server::broadcastScores()
{
    auto clients = _network->getConnectedClients();
    
    for (uint32_t client_id : clients) {
        Protocol::ScoreUpdate packet;
        packet.header.sequence_number = _sequenceCounter++;
        packet.client_id = client_id;
        packet.score = _game.getPlayerScore(client_id);
        packet.enemies_killed = _game.getPlayerKills(client_id);
        
        _network->broadcast(packet);
    }
}

void Server::broadcastGameOver()
{
    Protocol::GameOver packet;
    packet.header.sequence_number = _sequenceCounter++;
    
    auto finalScores = _game.getFinalScores();
    packet.num_players = static_cast<uint8_t>((std::min)(finalScores.size(), static_cast<size_t>(4)));
    
    for (size_t i = 0; i < packet.num_players; ++i) {
        packet.players[i] = finalScores[i];
    }
    
    std::cout << "[Server] Broadcasting GAME_OVER packet with " << (int)packet.num_players << " players" << std::endl;
    
    _network->broadcast(packet);
}

void Server::broadcastEvent(const LocalGameEvent& event)
{
    switch (event.type) {
        case EventType::ENTITY_SPAWNED: {
            Protocol::EntitySpawn packet;
            packet.header.sequence_number = _sequenceCounter++;
            packet.entity_id = event.entity_id;
            packet.entity_type = event.entity_type;
            packet.pos_x = event.pos_x;
            packet.pos_y = event.pos_y;
            _network->broadcast(packet);
            break;
        }
        
        case EventType::ENTITY_DESTROYED: {
            Protocol::EntityDestroy packet;
            packet.header.sequence_number = _sequenceCounter++;
            packet.entity_id = event.entity_id;
            packet.killer_id = event.related_id;
            packet.reason = event.extra_data;
            _network->broadcast(packet);
            break;
        }
        
        case EventType::ENTITY_FIRED: {
            Protocol::EntityFire packet;
            packet.header.sequence_number = _sequenceCounter++;
            packet.shooter_id = event.related_id;
            packet.projectile_id = event.entity_id;
            packet.pos_x = event.pos_x;
            packet.pos_y = event.pos_y;
            packet.projectile_type = event.extra_data;
            _network->broadcast(packet);
            break;
        }
        
        case EventType::PLAYER_DIED: {
            Protocol::PlayerDeath packet;
            packet.header.sequence_number = _sequenceCounter++;
            packet.player_id = event.entity_id;
            packet.killer_id = event.related_id;
            packet.death_type = event.extra_data;
            _network->broadcast(packet);
            break;
        }
    }
}

void Server::broadcastNetworkEvent(const RType::Protocol::GameEvent& event)
{
    RType::Protocol::GameEvent packet = event;
    packet.header.sequence_number = _sequenceCounter++;
    
    if (event.event_type == RType::Protocol::GameEventType::LEVEL_COMPLETE) {
        std::cout << "[Server] Broadcasting LEVEL_COMPLETE: " 
                  << event.levelName << " -> " << event.nextLevelName << std::endl;
    }
    
    _network->broadcast(packet);
}

}