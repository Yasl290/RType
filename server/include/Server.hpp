#pragma once

#include "protocol/Protocol.hpp"
#include "GameModule.hpp"
#include <thread>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace RType::Network {
    class NetworkModule;
    struct ReceivedMessage;
}

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
    void broadcastEvent(const LocalGameEvent& event);
    void broadcastNetworkEvent(const RType::Protocol::GameEvent& event);

    std::unique_ptr<RType::Network::NetworkModule> _network;
    
    GameModule _game;

    std::atomic<bool> _running;
    std::thread _game_thread;
    uint16_t _port;
    uint32_t _sequenceCounter;
    float _snapshotAccumulator;

    std::atomic<bool> _gameStarted;
    std::atomic<bool> _gameOver;

    std::unordered_map<uint32_t, bool> _readyByClient;
    mutable std::mutex _readyMutex;

    // Advanced Networking Features - Track #2
    // Delta Compression: Store last snapshot per client to send only changes
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, EntitySnapshot>> _lastSnapshotsByClient;
    std::mutex _snapshotMutex;

    // Rate Limiting: Track last input time per client (max 60/sec)
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> _lastInputTime;
    std::mutex _inputTimeMutex;
    static constexpr std::chrono::milliseconds MIN_INPUT_INTERVAL{16};  // ~60 FPS max

    static constexpr std::chrono::seconds CLIENT_TIMEOUT{30};
    static constexpr const char* SERVER_VERSION = "1.0.0";
};

} // namespace RType::Server