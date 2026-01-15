# R-Type Server Architecture

## Overview

The R-Type server implements a multi-layered architecture with clear separation of responsibilities between network management, game logic, and global orchestration.

```
┌─────────────────────────────────────────────┐
│            SERVER (Orchestration)           │
│  - Game Loop (60 FPS)                       │
│  - Message dispatching                      │
│  - Client/Server coordination               │
│  - Lobby management                         │
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│         NETWORK MODULE (UDP Layer)          │
│  - Non-blocking reception thread            │
│  - Thread-safe message queue                │
│  - Client list management                   │
│  - Timeout detection                        │
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│         GAME MODULE (ECS Engine)            │
│  - Entities and components                  │
│  - Gameplay systems                         │
│  - Snapshot generation                      │
└─────────────────────────────────────────────┘
```

---

## Network Module

### Responsibilities

The `NetworkModule` handles all low-level UDP communication:
- Asynchronous UDP packet reception
- Packet format validation
- Thread-safe queuing
- Packet sending (unicast, broadcast)
- Connected client list management
- Player slot assignment (0-3)
- Inactive client detection (timeout)

### Multi-Thread Architecture

#### Main Thread (Game Loop)
- Consumes messages from the queue
- Sends packets to clients
- Accesses client structures (read/write)

#### Network Thread (Reception Loop)
- Non-blocking reception on UDP socket
- Incoming packet validation
- Message production into the queue
- Activity timestamp updates

### Data Structures

#### Double Client Indexing

```cpp
std::unordered_map<Address, std::shared_ptr<Client>> _clients_by_addr;
std::unordered_map<uint32_t, std::shared_ptr<Client>> _clients_by_id;
```

**Rationale:**
- **By address**: Fast lookup during UDP reception (we know the source IP:port)
- **By ID**: Used throughout business logic (more readable and stable)

#### Message Queue (Producer-Consumer)

```cpp
std::queue<ReceivedMessage> _message_queue;
std::mutex _queue_mutex;
```

**Pattern:**
- **Producer**: Network thread enqueues received packets
- **Consumer**: Game loop dequeues and processes messages
- **Synchronization**: Mutex protects concurrent access

#### Slot Management

```cpp
bool _player_slots[4];  // true = occupied, false = free
std::mutex _slots_mutex;
```

Each player is assigned a slot (0-3) which determines:
- Vertical spawn position: `Y = 150 + (slot * 120)`
- Visual identity in game
- Network ID: `client_id = slot + 1`

### Reception Loop

```cpp
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
        
        if (bytes < sizeof(Protocol::PacketHeader)) {
            continue;  // Invalid packet
        }
        
        processRawPacket(buffer, bytes, from);
    }
}
```

**Characteristics:**
- **Non-blocking socket**: `receiveFrom` returns immediately
- 1ms sleep if no data: avoids 100% CPU usage
- Early validation: rejects packets that are too small
- No heavy processing: delegated to `processRawPacket`

### Packet Processing

```cpp
void NetworkModule::processRawPacket(buffer, size, from)
{
    // 1. Header casting
    const PacketHeader& header = *(PacketHeader*)buffer.data();
    
    // 2. Validation (magic number, size consistency)
    if (!validatePacket(header, size)) return;
    
    // 3. Client activity update (timestamp)
    updateClientActivity(from);
    
    // 4. Structured message creation
    ReceivedMessage msg;
    msg.packet_type = header.type;
    msg.payload_size = size;
    msg.payload = buffer;
    msg.source_addr = from;
    msg.client_id = getClientIdByAddress(from);
    
    // 5. Thread-safe enqueuing
    {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        _message_queue.push(msg);
    }
}
```

### Packet Validation

```cpp
bool NetworkModule::validatePacket(const PacketHeader& header, size_t size)
{
    // Magic number
    if (header.magic != 0x52545950) return false;
    
    // Minimum size
    if (size < sizeof(PacketHeader)) return false;
    
    // Announced size consistency
    size_t expected = sizeof(PacketHeader) + header.payload_size;
    if (size != expected) return false;
    
    return true;
}
```

### Timeout Detection

```cpp
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
```

**Mechanism:**
- Each packet received from a client updates its `_last_activity` timestamp
- `checkTimeouts` compares `now - _last_activity` with the threshold (e.g., 10s)
- Returns the list of IDs to disconnect

### Public API

#### Start/Stop

```cpp
bool start(uint16_t port);  // Bind socket, launch network thread
void stop();                 // Stop thread, close socket
```

#### Message Consumption

```cpp
std::vector<ReceivedMessage> pollMessages();
```

Returns all pending messages (complete queue drain).

#### Packet Sending

```cpp
// To a specific client (by ID)
template<typename T>
void sendToClient(uint32_t client_id, const T& packet);

// To an address (for CONNECT_RESPONSE before registration)
template<typename T>
void sendToAddress(const Address& addr, const T& packet);

// Broadcast to all connected clients
template<typename T>
void broadcast(const T& packet);

// Broadcast except one client (for notifications)
template<typename T>
void broadcastExcept(uint32_t exclude_id, const T& packet);
```

#### Client Management

```cpp
uint32_t registerClient(const Address& addr, const std::string& name, uint8_t slot);
void disconnectClient(uint32_t client_id);
std::shared_ptr<Client> getClient(uint32_t client_id) const;
std::shared_ptr<Client> getClientByAddress(const Address& addr) const;
std::vector<uint32_t> getConnectedClients() const;
size_t getClientCount() const;
```

#### Slot Management

```cpp
bool hasAvailableSlot() const;
uint8_t findFreePlayerSlot() const;  // Returns 255 if no free slot
```

---

## Server (Orchestrator)

### Responsibilities

The `Server` coordinates the entire system:
- Runs the main game loop (60 FPS)
- Dispatches network messages to appropriate handlers
- Manages game lifecycle (lobby → game → game over)
- Synchronizes state between clients
- Orchestrates NetworkModule and GameModule

### Game Loop

```cpp
void Server::gameLoopThread()
{
    constexpr auto TICK_RATE = milliseconds(16);      // 60 FPS
    constexpr float FIXED_DT = 1.0f / 60.0f;          // Physics delta time
    constexpr float SNAPSHOT_DT = 1.0f / 20.0f;       // 20 snapshots/sec
    
    auto next_tick = clock::now();
    
    while (_running) {
        auto now = clock::now();
        
        if (now >= next_tick) {
            // PHASE 1: Network message processing
            auto messages = _network.pollMessages();
            for (const auto& msg : messages) {
                handleMessage(msg);
            }

            // PHASE 2: Game update
            if (_gameStarted && !_gameOver) {
                _game.update(FIXED_DT);
                checkGameOver();
                
                // PHASE 3: Periodic snapshots
                _snapshotAccumulator += FIXED_DT;
                if (_snapshotAccumulator >= SNAPSHOT_DT) {
                    broadcastGameState();
                    broadcastScores();
                    _snapshotAccumulator = 0.f;
                }
            }
            
            // PHASE 4: Timeouts
            checkTimeouts();
            
            next_tick += TICK_RATE;
        } else {
            std::this_thread::sleep_for(milliseconds(1));
        }
    }
}
```

**Temporal characteristics:**
- **60 FPS** for physics and collisions (smoothness)
- **20 Hz** for network snapshots (bandwidth economy)
- Accumulation for precise send synchronization

### Message Dispatching

```cpp
void Server::handleMessage(const Network::ReceivedMessage& msg)
{
    switch (msg.packet_type) {
        case CONNECT_REQUEST:   handleConnectRequest(msg);   break;
        case DISCONNECT:        handleDisconnect(msg);       break;
        case PLAYER_INPUT:      handlePlayerInput(msg);      break;
        case READY_TO_PLAY:     handleReadyToPlay(msg);      break;
        case GAME_ON:           handleGameOn(msg);           break;
        case PING:              handlePing(msg);             break;
        default:
            std::cerr << "Unknown packet type" << std::endl;
    }
}
```

### Connection Flow

```
Client                          Server                     Other Clients
  │                               │                               │
  ├─ CONNECT_REQUEST ────────────>│                               │
  │  (player_name)                │                               │
  │                               ├─ Version validation           │
  │                               ├─ Capacity check               │
  │                               ├─ Slot assignment              │
  │                               ├─ registerClient()             │
  │                               ├─ spawnPlayer()                │
  │<─ CONNECT_RESPONSE ───────────┤                               │
  │  (client_id, slot)            │                               │
  │                               ├─ DISCONNECT (notify) ────────>│
  │                               │                               │
  │<─ LOBBY_STATUS ───────────────┼───────────────────────────────>│
  │  (players connected, ready)   │                               │
```

### Disconnection Flow

```cpp
void Server::handleDisconnect(const ReceivedMessage& msg)
{
    DisconnectPacket packet;
    std::memcpy(&packet, msg.payload.data(), sizeof(packet));
    
    // 1. Game cleanup
    _game.removePlayer(packet.client_id);
    _readyByClient.erase(packet.client_id);
    
    // 2. Network cleanup
    _network.disconnectClient(packet.client_id);
    
    // 3. Game reset if no more clients
    if (_network.getClientCount() == 0 && (_gameStarted || _gameOver)) {
        _gameStarted = false;
        _gameOver = false;
        _game.init();
    }
    
    // 4. Notification
    _network.broadcastExcept(packet.client_id, packet);
    broadcastLobbyStatus();
}
```

### Lobby Management

#### "Ready" Status

```cpp
std::unordered_map<uint32_t, bool> _readyByClient;
```

Each client must send a `READY_TO_PLAY` before starting.

```cpp
void Server::handleReadyToPlay(const ReceivedMessage& msg)
{
    ReadyToPlay packet;
    std::memcpy(&packet, msg.payload.data(), sizeof(packet));
    
    _readyByClient[packet.client_id] = (packet.ready != 0);
    
    // Lobby status broadcast
    broadcastLobbyStatus();
    
    // Automatic start if all ready
    if (!_gameStarted && !_gameOver && areAllPlayersReady()) {
        _gameStarted = true;
        
        GameOn start;
        _network.broadcast(start);
        broadcastLobbyStatus();
    }
}
```

#### Status Broadcasting

```cpp
void Server::broadcastLobbyStatus()
{
    LobbyStatus status;
    status.max_players = 4;
    status.players_connected = _network.getClientCount();
    status.players_ready = 0;
    status.ready_mask = 0;
    status.game_started = _gameStarted ? 1 : 0;
    
    // Binary mask construction for ready players
    for (uint32_t id : _network.getConnectedClients()) {
        auto client = _network.getClient(id);
        uint8_t slot = client->getPlayerSlot();
        
        if (_readyByClient[id]) {
            status.players_ready++;
            status.ready_mask |= (1 << slot);
        }
    }
    
    _network.broadcast(status);
}
```

**Ready Mask:**
```
Bit 0 = Slot 0 ready
Bit 1 = Slot 1 ready
Bit 2 = Slot 2 ready
Bit 3 = Slot 3 ready

Example: 0b00000101 = Slots 0 and 2 are ready
```

### Input Processing

```cpp
void Server::handlePlayerInput(const ReceivedMessage& msg)
{
    if (!_gameStarted) return;  // Ignore inputs outside of game
    
    PlayerInput input;
    std::memcpy(&input, msg.payload.data(), sizeof(input));
    
    // Delegation to GameModule
    _game.processInput(input.client_id, input.input_flags);
}
```

**Server Authority Principle:**
- Client sends: "I want to go left"
- Server calculates: new position, collision, etc.
- Next snapshot (~50ms) informs all clients of the resulting state

### World State Broadcasting

```cpp
void Server::broadcastGameState()
{
    if (_network.getClientCount() == 0) return;
    
    auto snapshots = _game.getWorldSnapshot();
    
    for (const auto& snap : snapshots) {
        EntityUpdate packet;
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
```

**Snapshot contents:**
- ALL players (positions, HP, velocities)
- ALL active enemies
- ALL projectiles in flight
- Frequency: 20 times/second

### Game Over

```cpp
void Server::checkGameOver()
{
    if (_game.areAllPlayersDead()) {
        _gameOver = true;
        _gameStarted = false;
        broadcastGameOver();
    }
}

void Server::broadcastGameOver()
{
    GameOver packet;
    
    auto finalScores = _game.getFinalScores();
    packet.num_players = std::min(finalScores.size(), 4);
    
    for (size_t i = 0; i < packet.num_players; ++i) {
        packet.players[i] = finalScores[i];
    }
    
    _network.broadcast(packet);
}
```

**Complete lifecycle:**
```
[Lobby] → [All Ready] → [Game Started] → [All Dead] → [Game Over] → [Lobby]
```

Clients can stay connected and restart a game.

---

## Thread-Safe Synchronization

### Mutexes Used

```cpp
// NetworkModule
std::mutex _clients_mutex;   // Protects client maps
std::mutex _queue_mutex;     // Protects message queue
std::mutex _slots_mutex;     // Protects slot array
```

### Lock Guard Pattern

```cpp
{
    std::lock_guard<std::mutex> lock(_clients_mutex);
    // Critical section
    auto it = _clients_by_id.find(id);
    // Automatic unlock at scope exit
}
```

### Critical Sections

| Structure              | Reader Thread     | Writer Thread     | Protection           |
|------------------------|-------------------|-------------------|----------------------|
| `_clients_by_id`       | Game Loop         | Network Thread    | `_clients_mutex`     |
| `_clients_by_addr`     | Network Thread    | Game Loop         | `_clients_mutex`     |
| `_message_queue`       | Game Loop (pop)   | Network (push)    | `_queue_mutex`       |
| `_player_slots`        | Game Loop         | Game Loop         | `_slots_mutex`       |

---

## Sequence Diagrams

### Connection and Spawn

```
Client                  NetworkModule              Server                GameModule
  │                          │                       │                       │
  ├─ CONNECT_REQUEST ───────>│                       │                       │
  │                          ├─ Validation           │                       │
  │                          ├─ Enqueue message ────>│                       │
  │                          │                       ├─ handleConnect()      │
  │                          │                       ├─ registerClient() ────>│
  │                          │<────────────────────────┤                      │
  │<─ CONNECT_RESPONSE ──────┤                       ├─ spawnPlayer() ──────>│
  │                          │                       │                       ├─ Create Entity
  │                          │                       │<──────────────────────┤
  │                          │<─ broadcastLobby ─────┤                       │
  │<─ LOBBY_STATUS ──────────┤                       │                       │
```

### Game Loop and Snapshots

```
NetworkModule          Server                 GameModule
      │                  │                        │
      │─ pollMessages()─>│                        │
      │<── messages ─────┤                        │
      │                  ├─ handleInput() ───────>│
      │                  │                        ├─ Apply movement
      │                  ├─ update(dt) ──────────>│
      │                  │                        ├─ Physics
      │                  │                        ├─ Collisions
      │                  │<─ events ──────────────┤
      │                  ├─ getWorldSnapshot() ──>│
      │                  │<─ entities[] ──────────┤
      │<─ broadcast() ───┤                        │
      │                  │                        │
   [50ms]                │                        │
      │                  ├─ broadcastState()      │
      │<─ EntityUpdate ──┤ (20 Hz)               │
```

---

## Configuration and Constants

### Frequencies

```cpp
constexpr auto TICK_RATE = milliseconds(16);     // 60 FPS
constexpr float FIXED_DT = 1.0f / 60.0f;         // 0.0166s
constexpr float SNAPSHOT_DT = 1.0f / 20.0f;      // 0.05s (20 Hz)
```

### Timeouts

```cpp
constexpr auto CLIENT_TIMEOUT = std::chrono::seconds(10);
```

A client that sends no packets for 10 seconds is disconnected.

### Sizes

```cpp
constexpr size_t MAX_PACKET_SIZE = 1024;    // UDP buffer
constexpr uint8_t MAX_PLAYERS = 4;          // Slots 0-3
```

---

## Error Handling

### Invalid Packets

```cpp
// Insufficient size
if (bytes < sizeof(PacketHeader)) {
    std::cerr << "Packet too small" << std::endl;
    continue;
}

// Incorrect magic number
if (header.magic != 0x52545950) {
    std::cerr << "Invalid magic" << std::endl;
    return false;
}

// Size inconsistency
if (received_size != expected_size) {
    std::cerr << "Size mismatch" << std::endl;
    return false;
}
```

### Unknown Clients

```cpp
auto client = _network.getClient(client_id);
if (!client) {
    return;  // Silently ignore
}
```

### Server Full

```cpp
if (!_network.hasAvailableSlot()) {
    response.status = ConnectionStatus::REJECTED_FULL;
    _network.sendToAddress(addr, response);
    return;
}
```

---

## Optimizations

### Bandwidth Reduction

1. **20 Hz snapshots** instead of 60 FPS: `-66% traffic`
2. **Packed structures**: no unnecessary padding
3. **Selective broadcast**: `broadcastExcept` avoids echoes

### CPU Performance

1. **Non-blocking socket**: no active waiting
2. **1ms sleep**: avoids 100% CPU in idle
3. **Early validation**: quickly rejects invalid packets

### Scalability

1. **Double indexing**: O(1) lookup by ID and by address
2. **Message queue**: decoupling network/game loop
3. **Separate thread**: I/O and CPU in parallel

---

## Deployment

### Starting

```cpp
Server server(PORT);
server.start();  // Non-blocking, launches threads

// Server runs in background
std::this_thread::sleep_for(std::chrono::hours(24));

server.stop();   // Clean stop
```

### Typical Logs

```
[NetworkModule] Started on port 8080
Server started on port 8080
Client connected: Player1 [192.168.1.42:5678] ID=1 Slot=0
Client connected: Player2 [192.168.1.43:5679] ID=2 Slot=1
[Server] All players ready - starting game
[Server] Game Over! All players dead.
Client disconnected: Player1 [192.168.1.42:5678]
[Server] All clients disconnected - resetting game
```

---

## Conclusion

The R-Type server architecture is based on three pillars:

1. **Separation of concerns**: NetworkModule (I/O), Server (orchestration), GameModule (logic)
2. **Server Authority**: clients send inputs, server computes results, snapshots synchronize
3. **Performance**: multi-threading, non-blocking I/O, optimized frequencies

This architecture guarantees:
-  Game state consistency between all clients
-  Robustness against latency and packet loss
-  Disconnection detection and handling
-  Scalability up to 4 simultaneous players
-  Maintainable and extensible code