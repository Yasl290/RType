#pragma once
#include "NetworkModule.hpp"
#include "protocol/Protocol.hpp"
#include "GameModule.hpp"
#include <thread>
#include <atomic>
#include <memory>
#include <unordered_map>

namespace RType::Server {

class Server {
public:
    Server(uint16_t port);
    ~Server();
    
    void start();
    void stop();
    
private:
    void gameLoopThread();
    
    void handleMessage(const RType::Network::ReceivedMessage& msg);
    void handleConnectRequest(const RType::Network::ReceivedMessage& msg);
    void handleDisconnect(const RType::Network::ReceivedMessage& msg);
    void handlePlayerInput(const RType::Network::ReceivedMessage& msg);
    void handleReadyToPlay(const RType::Network::ReceivedMessage& msg);
    void handleGameOn(const RType::Network::ReceivedMessage& msg);
    void handlePing(const RType::Network::ReceivedMessage& msg);

    void checkTimeouts();
    void broadcastGameState();
    void broadcastLobbyStatus();
    bool areAllPlayersReady() const;
    void broadcastScores();
    void checkGameOver();
    void broadcastGameOver();
    void broadcastEvent(const GameEvent& event);

    
    RType::Network::NetworkModule _network;
    GameModule _game;
    
    std::atomic<bool> _running;
    std::thread _game_thread;
    
    uint16_t _port;
    uint32_t _sequenceCounter;
    float _snapshotAccumulator;

    bool _gameStarted;
    bool _gameOver;

    std::unordered_map<uint32_t, bool> _readyByClient;
    
    static constexpr std::chrono::seconds CLIENT_TIMEOUT{30};
    static constexpr const char* SERVER_VERSION = "1.0.0";
};

} // namespace RType::Server