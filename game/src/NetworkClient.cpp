#include "NetworkClient.hpp"
#include "protocol/Protocol.hpp"
#include <array>
#include <chrono>
#include <iostream>

#ifdef _WIN32
typedef int ssize_t;
#else
#include <fcntl.h>
#endif

static constexpr auto CONNECT_TIMEOUT = std::chrono::seconds(5);
static constexpr auto RECEIVE_SLEEP = std::chrono::milliseconds(1);
static constexpr auto HEARTBEAT_INTERVAL = std::chrono::seconds(2);
static constexpr auto SERVER_TIMEOUT = std::chrono::seconds(10);

static std::unordered_map<uint32_t, bool> s_playerEntityIds;

static bool waitForReadable(int fd, std::chrono::milliseconds timeout) {
  fd_set readfds;
  timeval tv;
  auto millis = timeout.count();

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);

  tv.tv_sec = static_cast<long>(millis / 1000);
#ifdef _WIN32
  tv.tv_usec = static_cast<long>((millis % 1000) * 1000);
#else
  tv.tv_usec = static_cast<suseconds_t>((millis % 1000) * 1000);
#endif

  int ready = select(fd + 1, &readfds, nullptr, nullptr, &tv);
  return ready > 0 && FD_ISSET(fd, &readfds);
}

static bool sendPacket(int sockfd, const sockaddr_in &addr, const void *data,
                       size_t size) {
#ifdef _WIN32
  if (sockfd == INVALID_SOCKET) {
#else
  if (sockfd < 0) {
#endif
    return false;
  }

#ifdef _WIN32
  ssize_t sent =
      sendto(sockfd, (const char *)data, (int)size, 0,
             reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
#else
  ssize_t sent =
      sendto(sockfd, data, size, 0, reinterpret_cast<const sockaddr *>(&addr),
             sizeof(addr));
#endif
  return sent == static_cast<ssize_t>(size);
}

NetworkClient::NetworkClient() 
    : _sockfd(-1)
    , _clientId(0)
    , _playerSlot(0)
    , _connected(false)
    , _running(false)
    , _hasLobbyStatus(false)
    , _gameOn(false)
    , _gameOver(false)
    , _sequence(0)
    , _lastHeartbeat(std::chrono::steady_clock::now())
    , _lastServerActivity(std::chrono::steady_clock::now())
    , _soloMode(false)
{
#ifdef _WIN32
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
  std::memset(&_serverAddr, 0, sizeof(_serverAddr));
}

NetworkClient::~NetworkClient() {
  disconnect();
#ifdef _WIN32
  WSACleanup();
#endif
}

bool NetworkClient::connect(const std::string &server_ip, uint16_t port,
                            const std::string &player_name) {
  sockaddr_in from_addr{};
  socklen_t from_len = sizeof(from_addr);
  ssize_t received = 0;
  auto timeoutMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(CONNECT_TIMEOUT);
  RType::Protocol::ConnectRequest request;
  RType::Protocol::ConnectResponse response;

  if (_connected) {
    disconnect();
  }

  _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
  if (_sockfd == INVALID_SOCKET) {
#else
  if (_sockfd < 0) {
#endif
    std::cerr << "[NetworkClient] Failed to create socket" << std::endl;
    return false;
  }
  _serverAddr.sin_family = AF_INET;
  _serverAddr.sin_port = htons(port);
  if (inet_pton(AF_INET, server_ip.c_str(), &_serverAddr.sin_addr) <= 0) {
    std::cerr << "[NetworkClient] Invalid server address" << std::endl;
#ifdef _WIN32
    ::closesocket(_sockfd);
    _sockfd = INVALID_SOCKET;
#else
    ::close(_sockfd);
    _sockfd = -1;
#endif
    return false;
  }

  request.header.sequence_number = _sequence++;
  std::strncpy(request.client_version, "1.0.0",
               sizeof(request.client_version) - 1);
  std::strncpy(request.player_name, player_name.c_str(),
               sizeof(request.player_name) - 1);

  if (!sendPacket(_sockfd, _serverAddr, &request, sizeof(request))) {
    std::cerr << "[NetworkClient] Failed to send connect request" << std::endl;
#ifdef _WIN32
    ::closesocket(_sockfd);
    _sockfd = INVALID_SOCKET;
#else
    ::close(_sockfd);
    _sockfd = -1;
#endif
    return false;
  }

  if (!waitForReadable(_sockfd, timeoutMs)) {
    std::cerr << "[NetworkClient] Connection timeout" << std::endl;
#ifdef _WIN32
    ::closesocket(_sockfd);
    _sockfd = INVALID_SOCKET;
#else
    ::close(_sockfd);
    _sockfd = -1;
#endif
    return false;
  }

#ifdef _WIN32
  received = recvfrom(_sockfd, (char *)&response, (int)sizeof(response), 0,
                      (struct sockaddr *)&from_addr, &from_len);
#else
  received = recvfrom(_sockfd, &response, sizeof(response), 0,
                      (struct sockaddr *)&from_addr, &from_len);
#endif

  std::cout << "[NetworkClient] Received " << received << " bytes, expected "
            << sizeof(RType::Protocol::ConnectResponse) << std::endl;
  std::cout << "[NetworkClient] Magic: 0x" << std::hex << response.header.magic
            << std::dec << std::endl;
  std::cout << "[NetworkClient] Type: " << (int)response.header.type
            << std::endl;

  if (received < (ssize_t)sizeof(RType::Protocol::ConnectResponse) ||
      !RType::Protocol::IsValidPacket(response.header) ||
      response.header.type != RType::Protocol::PacketType::CONNECT_RESPONSE) {
    std::cerr << "[NetworkClient] Invalid response" << std::endl;
#ifdef _WIN32
    ::closesocket(_sockfd);
    _sockfd = INVALID_SOCKET;
#else
    ::close(_sockfd);
    _sockfd = -1;
#endif
    return false;
  }

  if (response.status != RType::Protocol::ConnectionStatus::ACCEPTED) {
    std::cerr << "[NetworkClient] Connection rejected" << std::endl;
#ifdef _WIN32
    ::closesocket(_sockfd);
    _sockfd = INVALID_SOCKET;
#else
    ::close(_sockfd);
    _sockfd = -1;
#endif
    return false;
  }
  _clientId = response.client_id;
  _playerSlot = response.assigned_player_slot;
  _connected = true;
  _hasLobbyStatus = false;
  _gameOn = false;

  _lastHeartbeat = std::chrono::steady_clock::now();
  _lastServerActivity = std::chrono::steady_clock::now();

  _running = true;
  _receiveThread = std::thread(&NetworkClient::receiveLoop, this);

  std::cout << "[NetworkClient] Connected! Client ID: " << _clientId
            << std::endl;

  return true;
}

void NetworkClient::disconnect() {
  if (!_connected) {
    return;
  }

  RType::Protocol::DisconnectPacket packet;
  packet.client_id = _clientId;
  packet.reason = 0;
  sendPacket(_sockfd, _serverAddr, &packet, sizeof(packet));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  _running = false;
  _connected = false;
  _hasLobbyStatus = false;
  _gameOn = false;

  if (_receiveThread.joinable()) {
    _receiveThread.join();
  }

#ifdef _WIN32
  if (_sockfd != INVALID_SOCKET) {
    ::closesocket(_sockfd);
    _sockfd = INVALID_SOCKET;
  }
#else
  if (_sockfd >= 0) {
    ::close(_sockfd);
    _sockfd = -1;
  }
#endif

  std::cout << "[NetworkClient] Disconnected" << std::endl;
}

void NetworkClient::sendInput(uint8_t input_flags) {
  if (!_connected) {
    return;
  }
  RType::Protocol::PlayerInput input;
  input.header.sequence_number = _sequence++;
  input.client_id = _clientId;
  input.input_flags = input_flags;
  input.timestamp = 0;

  sendPacket(_sockfd, _serverAddr, &input, sizeof(input));
}

void NetworkClient::sendReady(bool ready) {
  if (!_connected) {
    return;
  }

  RType::Protocol::ReadyToPlay packet;
  packet.header.sequence_number = _sequence++;
  packet.client_id = _clientId;
  packet.ready = ready ? 1 : 0;

  sendPacket(_sockfd, _serverAddr, &packet, sizeof(packet));
}

LobbyStatusSnapshot NetworkClient::getLobbyStatus() const {
  std::lock_guard<std::mutex> lock(_lobbyMutex);
  return _lobbyStatus;
}

std::vector<ReceivedEntity> NetworkClient::pollEntityUpdates() {
  std::lock_guard<std::mutex> lock(_queueMutex);

  std::vector<ReceivedEntity> entities;
  entities.reserve(_entityQueue.size());
  while (!_entityQueue.empty()) {
    entities.push_back(_entityQueue.front());
    _entityQueue.pop();
  }

  return entities;
}

std::vector<RType::Protocol::GameEvent> NetworkClient::pollGameEvents() {
  std::lock_guard<std::mutex> lock(_queueMutex);

  std::vector<RType::Protocol::GameEvent> events;
  events.reserve(_gameEventQueue.size());
  while (!_gameEventQueue.empty()) {
    events.push_back(_gameEventQueue.front());
    _gameEventQueue.pop();
  }

  return events;
}

std::vector<RType::Protocol::EntityFire> NetworkClient::pollFireEvents() {
  std::lock_guard<std::mutex> lock(_queueMutex);

  std::vector<RType::Protocol::EntityFire> events;
  events.reserve(_fireEventsQueue.size());
  while (!_fireEventsQueue.empty()) {
    events.push_back(_fireEventsQueue.front());
    _fireEventsQueue.pop();
  }

  return events;
}

void NetworkClient::receiveLoop() {
  std::array<uint8_t, 2048> buffer{};
#ifdef _WIN32
  u_long mode = 1;
  ioctlsocket(_sockfd, FIONBIO, &mode);
#else
  int flags = fcntl(_sockfd, F_GETFL, 0);
  if (flags >= 0) {
    fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK);
  }
#endif

  while (_running) {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastHeartbeat =
        std::chrono::duration_cast<std::chrono::seconds>(now - _lastHeartbeat);

    if (timeSinceLastHeartbeat >= HEARTBEAT_INTERVAL) {
      RType::Protocol::PacketHeader ping;
      ping.type = RType::Protocol::PacketType::PING;
      ping.sequence_number = _sequence++;
      sendPacket(_sockfd, _serverAddr, &ping, sizeof(ping));
      _lastHeartbeat = now;
      std::cout << "[NetworkClient] Sent heartbeat ping" << std::endl;
    }

    auto timeSinceServerActivity =
        std::chrono::duration_cast<std::chrono::seconds>(now -
                                                         _lastServerActivity);

    if (timeSinceServerActivity >= SERVER_TIMEOUT) {
      static bool timeoutLogged = false;
      if (!timeoutLogged) {
        std::cerr << "[NetworkClient] ⚠️  Server not responding for "
                  << timeSinceServerActivity.count() << " seconds (waiting...)"
                  << std::endl;
        timeoutLogged = true;
      }
      if (timeSinceServerActivity < SERVER_TIMEOUT) {
        timeoutLogged = false;
      }
    }

    sockaddr_in from_addr{};
    socklen_t from_len = sizeof(from_addr);

#ifdef _WIN32
    ssize_t received =
        recvfrom(_sockfd, (char *)buffer.data(), (int)buffer.size(), 0,
                 (struct sockaddr *)&from_addr, &from_len);
#else
    ssize_t received = recvfrom(_sockfd, buffer.data(), buffer.size(), 0,
                                (struct sockaddr *)&from_addr, &from_len);
#endif

    if (received > 0) {
      _lastServerActivity = now;
      handlePacket(buffer.data(), static_cast<size_t>(received));
    }

    std::this_thread::sleep_for(RECEIVE_SLEEP);
  }
}

void NetworkClient::handlePacket(const uint8_t* data, size_t size)
{
    if (size < sizeof(RType::Protocol::PacketHeader)) {
        return;
    }
    
    RType::Protocol::PacketHeader header;
    std::memcpy(&header, data, sizeof(RType::Protocol::PacketHeader));
    
    if (!RType::Protocol::IsValidPacket(header)) {
        return;
    }
    
    switch (header.type) {
case RType::Protocol::PacketType::ENTITY_UPDATE: {

    if (size >= sizeof(RType::Protocol::PacketHeader) + sizeof(uint16_t)) {
        uint16_t entity_count = 0;
        std::memcpy(&entity_count, data + sizeof(RType::Protocol::PacketHeader), sizeof(uint16_t));

        if (entity_count > 0 && entity_count <= 64) {
            size_t expected_size = sizeof(RType::Protocol::PacketHeader) + sizeof(uint16_t) +
                                  (entity_count * sizeof(RType::Protocol::QuantizedEntityData));

            if (size >= expected_size) {
                const uint8_t* entity_data = data + sizeof(RType::Protocol::PacketHeader) + sizeof(uint16_t);

                std::lock_guard<std::mutex> lock(_queueMutex);

                static int bossUpdateCounter = 0;

                for (uint16_t i = 0; i < entity_count; ++i) {
                    RType::Protocol::QuantizedEntityData qdata;
                    std::memcpy(&qdata, entity_data + (i * sizeof(RType::Protocol::QuantizedEntityData)),
                               sizeof(RType::Protocol::QuantizedEntityData));

                    ReceivedEntity entity;
                    entity.entity_id = qdata.entity_id;
                    entity.pos_x = RType::Protocol::dequantizePosition(qdata.pos_x);
                    entity.pos_y = RType::Protocol::dequantizePosition(qdata.pos_y);
                    entity.vel_x = RType::Protocol::dequantizeVelocity(qdata.vel_x);
                    entity.vel_y = RType::Protocol::dequantizeVelocity(qdata.vel_y);
                    entity.hp_current = RType::Protocol::dequantizeHP(qdata.hp_current);
                    entity.hp_max = RType::Protocol::dequantizeHP(qdata.hp_max);
                    entity.entity_type = qdata.entity_type;
                    entity.player_slot = qdata.player_slot;

                    if (entity.entity_type == 5) {
                        if (bossUpdateCounter % 60 == 0) {
                            std::cout << "[NetworkClient] Boss BATCHED_UPDATE received: ID=" << entity.entity_id
                                      << " Type=" << (int)entity.entity_type
                                      << " Pos=(" << entity.pos_x << "," << entity.pos_y << ")"
                                      << " HP=" << entity.hp_current << "/" << entity.hp_max << std::endl;
                        }
                        bossUpdateCounter++;
                    }

                    _entityQueue.push(entity);
                }

                break;
            }
        }
    }

    if (size >= sizeof(RType::Protocol::EntityUpdate)) {
        RType::Protocol::EntityUpdate update;
        std::memcpy(&update, data, sizeof(RType::Protocol::EntityUpdate));

        ReceivedEntity entity;
        entity.entity_id = update.entity_id;
        entity.pos_x = update.pos_x;
        entity.pos_y = update.pos_y;
        entity.vel_x = update.vel_x;
        entity.vel_y = update.vel_y;
        entity.entity_type = update.entity_type;
        entity.hp_current = update.hp_current;
        entity.hp_max = update.hp_max;
        entity.player_slot = update.player_slot;

        static int bossUpdateCounter = 0;
        if (update.entity_type == 5) {
            if (bossUpdateCounter % 60 == 0) {
                std::cout << "[NetworkClient] Boss ENTITY_UPDATE (legacy) received: ID=" << update.entity_id
                          << " Type=" << (int)update.entity_type
                          << " Pos=(" << update.pos_x << "," << update.pos_y << ")"
                          << " HP=" << update.hp_current << "/" << update.hp_max << std::endl;
            }
            bossUpdateCounter++;
        }

        std::lock_guard<std::mutex> lock(_queueMutex);
        _entityQueue.push(entity);
    }
    break;
}

  case RType::Protocol::PacketType::ENTITY_SPAWN: {
    if (size >= sizeof(RType::Protocol::EntitySpawn)) {
      RType::Protocol::EntitySpawn spawn;
      std::memcpy(&spawn, data, sizeof(RType::Protocol::EntitySpawn));

      std::cout << "[NetworkClient] Entity spawned: ID=" << spawn.entity_id
                << " Type=" << (int)spawn.entity_type << " Pos=(" << spawn.pos_x
                << "," << spawn.pos_y << ")" << std::endl;

      if (spawn.entity_type ==
          static_cast<uint8_t>(RType::Protocol::EntityType::PLAYER)) {
        s_playerEntityIds[spawn.entity_id] = true;
      }
    }
    break;
  }

  case RType::Protocol::PacketType::ENTITY_DESTROY: {
    if (size >= sizeof(RType::Protocol::EntityDestroy)) {
      RType::Protocol::EntityDestroy destroy;
      std::memcpy(&destroy, data, sizeof(RType::Protocol::EntityDestroy));

      std::cout << "[NetworkClient] Entity destroyed: ID=" << destroy.entity_id
                << " Killer=" << destroy.killer_id << std::endl;

      s_playerEntityIds.erase(destroy.entity_id);
    }
    break;
  }

  case RType::Protocol::PacketType::ENTITY_FIRE: {
    if (size >= sizeof(RType::Protocol::EntityFire)) {
      RType::Protocol::EntityFire fire;
      std::memcpy(&fire, data, sizeof(RType::Protocol::EntityFire));

      std::cout << "[NetworkClient] Entity fired: Shooter=" << fire.shooter_id
                << " Projectile=" << fire.projectile_id
                << " Type=" << (int)fire.projectile_type << std::endl;

      if (s_playerEntityIds.find(fire.shooter_id) != s_playerEntityIds.end()) {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _fireEventsQueue.push(fire);
      }
    }
    break;
  }

  case RType::Protocol::PacketType::PLAYER_DEATH: {
    if (size >= sizeof(RType::Protocol::PlayerDeath)) {
      RType::Protocol::PlayerDeath death;
      std::memcpy(&death, data, sizeof(RType::Protocol::PlayerDeath));

      std::cout << "[NetworkClient] Player died: ID=" << death.player_id
                << " Killer=" << death.killer_id
                << " Type=" << (int)death.death_type << std::endl;
    }
    break;
  }

  case RType::Protocol::PacketType::GAME_EVENT: {
    if (size >= sizeof(RType::Protocol::GameEvent)) {
      RType::Protocol::GameEvent evt;
      std::memcpy(&evt, data, sizeof(RType::Protocol::GameEvent));

      std::lock_guard<std::mutex> lock(_queueMutex);
      _gameEventQueue.push(evt);

      if (evt.event_type == RType::Protocol::GameEventType::LEVEL_COMPLETE) {
        std::cout << "[NetworkClient] Level transition event: " << evt.levelName
                  << " -> " << evt.nextLevelName << std::endl;
      }
    }
    break;
  }

  case RType::Protocol::PacketType::LOBBY_STATUS: {
    if (size >= sizeof(RType::Protocol::LobbyStatus)) {
      RType::Protocol::LobbyStatus status;
      std::memcpy(&status, data, sizeof(RType::Protocol::LobbyStatus));

      LobbyStatusSnapshot snap;
      snap.max_players = status.max_players;
      snap.players_connected = status.players_connected;
      snap.players_ready = status.players_ready;
      snap.ready_mask = status.ready_mask;
      snap.game_started = status.game_started != 0;

      {
        std::lock_guard<std::mutex> lock(_lobbyMutex);
        _lobbyStatus = snap;
        _hasLobbyStatus = true;
      }
    }
    break;
  }

  case RType::Protocol::PacketType::GAME_ON: {
    _gameOn = true;
    break;
  }

  case RType::Protocol::PacketType::SCORE_UPDATE: {
    if (size >= sizeof(RType::Protocol::ScoreUpdate)) {
      RType::Protocol::ScoreUpdate update;
      std::memcpy(&update, data, sizeof(RType::Protocol::ScoreUpdate));

      std::lock_guard<std::mutex> lock(_scoreMutex);
      _playerScores[update.client_id] = {update.client_id, update.score,
                                         update.enemies_killed};
    }
    break;
  }

  case RType::Protocol::PacketType::GAME_OVER: {
    if (size >= sizeof(RType::Protocol::GameOver)) {
      RType::Protocol::GameOver packet;
      std::memcpy(&packet, data, sizeof(RType::Protocol::GameOver));

      std::lock_guard<std::mutex> lock(_gameOverMutex);
      _gameOver = true;
      _finalScores.clear();

      for (uint8_t i = 0; i < packet.num_players; ++i) {
        _finalScores.push_back(packet.players[i]);
      }

      std::cout << "[NetworkClient] Game Over received with "
                << (int)packet.num_players << " players!" << std::endl;
    }
    break;
  }

  case RType::Protocol::PacketType::PING: {
    RType::Protocol::PacketHeader pong;
    pong.type = RType::Protocol::PacketType::PONG;
    pong.sequence_number = header.sequence_number;

    sendPacket(_sockfd, _serverAddr, &pong, sizeof(pong));
    break;
  }

  case RType::Protocol::PacketType::PONG: {
    std::cout << "[NetworkClient] Received pong from server" << std::endl;
    break;
  }

  default:
    break;
  }
}

std::unordered_map<uint32_t, PlayerScore>
NetworkClient::getPlayerScores() const {
  std::lock_guard<std::mutex> lock(_scoreMutex);
  return _playerScores;
}

bool NetworkClient::isGameOver() const {
  std::lock_guard<std::mutex> lock(_gameOverMutex);
  return _gameOver;
}

std::vector<RType::Protocol::PlayerFinalScore>
NetworkClient::getFinalScores() const {
  std::lock_guard<std::mutex> lock(_gameOverMutex);
  return _finalScores;
}

void NetworkClient::resetGameOver() {
  std::lock_guard<std::mutex> lock(_gameOverMutex);
  _gameOver = false;
  _finalScores.clear();
}