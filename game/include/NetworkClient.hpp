#pragma once

#include "protocol/Protocol.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstdint>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <engine/gameplay/IScoreProvider.hpp>

struct ReceivedEntity {
    uint32_t entity_id;
    float pos_x;
    float pos_y;
    float vel_x;
    float vel_y;
    uint8_t entity_type;
    float hp_current;
    float hp_max;
    uint8_t player_slot;
};

struct LobbyStatusSnapshot {
    uint8_t max_players = 4;
    uint8_t players_connected = 0;
    uint8_t players_ready = 0;
    uint8_t ready_mask = 0;
    bool game_started = false;
};

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool connect(const std::string& server_ip, uint16_t port, const std::string& player_name);
    void disconnect();
    bool isConnected() const { return _connected; }

    void sendInput(uint8_t input_flags);
    void sendReady(bool ready);

    std::vector<ReceivedEntity> pollEntityUpdates();

    bool hasLobbyStatus() const { return _hasLobbyStatus; }
    LobbyStatusSnapshot getLobbyStatus() const;
    bool isGameOn() const { return _gameOn; }
    void clearGameOn() { _gameOn = false; }

    bool isGameOver() const;
    std::vector<RType::Protocol::PlayerFinalScore> getFinalScores() const;
    void resetGameOver();

    uint32_t getClientId() const { return _clientId; }
    uint8_t getPlayerSlot() const { return _playerSlot; }

    std::unordered_map<uint32_t, PlayerScore> getPlayerScores() const;

private:
    void receiveLoop();
    void handlePacket(const uint8_t* data, size_t size);

    int _sockfd;
    sockaddr_in _serverAddr;

    uint32_t _clientId;
    uint8_t _playerSlot;
    bool _connected;

    std::thread _receiveThread;
    std::atomic<bool> _running;

    std::queue<ReceivedEntity> _entityQueue;
    std::mutex _queueMutex;

    mutable std::mutex _lobbyMutex;
    LobbyStatusSnapshot _lobbyStatus;
    bool _hasLobbyStatus;

    std::unordered_map<uint32_t, PlayerScore> _playerScores;
    mutable std::mutex _scoreMutex;

    std::atomic<bool> _gameOn;
    bool _gameOver;
    std::vector<RType::Protocol::PlayerFinalScore> _finalScores;
    mutable std::mutex _gameOverMutex;

    uint32_t _sequence;
};