#pragma once

#include "protocol/Protocol.hpp"

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <engine/gameplay/IScoreProvider.hpp>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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

  bool connect(const std::string &server_ip, uint16_t port,
               const std::string &player_name);
  void disconnect();
  bool isConnected() const { return _connected.load(); }

  void sendInput(uint8_t input_flags);
  void sendReady(bool ready);

  std::vector<ReceivedEntity> pollEntityUpdates();
  std::vector<RType::Protocol::GameEvent> pollGameEvents();
  std::vector<RType::Protocol::EntityFire> pollFireEvents();

  bool hasLobbyStatus() const {
    std::lock_guard<std::mutex> lock(_lobbyMutex);
    return _hasLobbyStatus;
  }
  LobbyStatusSnapshot getLobbyStatus() const;
  bool isGameOn() const { return _gameOn.load(); }
  void clearGameOn() { _gameOn.store(false); }

  bool isGameOver() const;
  std::vector<RType::Protocol::PlayerFinalScore> getFinalScores() const;
  void resetGameOver();

  uint32_t getClientId() const { return _clientId.load(); }
  uint8_t getPlayerSlot() const { return _playerSlot.load(); }

  std::unordered_map<uint32_t, PlayerScore> getPlayerScores() const;

  bool isServerResponding() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       now - _lastServerActivity)
                       .count();
    return elapsed < 3;
  }

  void setSoloMode(bool solo) { _soloMode = solo; }
  bool isSoloMode() const { return _soloMode; }

private:
  void receiveLoop();
  void handlePacket(const uint8_t *data, size_t size);

  int _sockfd;
  sockaddr_in _serverAddr;

  std::atomic<uint32_t> _clientId;
  std::atomic<uint8_t> _playerSlot;
  std::atomic<bool> _connected;

  std::thread _receiveThread;
  std::atomic<bool> _running;

  std::queue<ReceivedEntity> _entityQueue;
  std::queue<RType::Protocol::GameEvent> _gameEventQueue;
  std::queue<RType::Protocol::EntityFire> _fireEventsQueue;
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

  std::atomic<uint32_t> _sequence;
  std::chrono::steady_clock::time_point _lastHeartbeat;
  std::chrono::steady_clock::time_point _lastServerActivity;
  bool _soloMode;
};