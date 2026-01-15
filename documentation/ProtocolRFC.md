# RFC - R-Type Network Protocol

**Version:** 1.0  
**Date:** December 2024  
**Status:** Final Specification  
**Author:** R-Type Team

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [General Architecture](#2-general-architecture)
3. [Packet Format](#3-packet-format)
4. [Packet Types](#4-packet-types)
5. [Communication Flows](#5-communication-flows)
6. [Error Handling](#6-error-handling)
7. [Security Considerations](#7-security-considerations)
8. [Appendices](#8-appendices)

---

## 1. Introduction

### 1.1 Purpose

This document specifies the network communication protocol for the R-Type multiplayer game, enabling real-time synchronization of up to 4 players via UDP.

### 1.2 Terminology

- **Client**: Game instance controlled by a player
- **Server**: Authoritative instance managing game state
- **Packet**: UDP data unit containing a header and payload
- **Snapshot**: Complete game state photo at a given time
- **Server Authority**: The server is the source of truth for all states

### 1.3 Conventions

The keywords "MUST", "MUST NOT", "REQUIRED", "SHALL", "MAY" follow RFC 2119 semantics.

---

## 2. General Architecture

### 2.1 Communication Model

**Transport Protocol:** UDP (User Datagram Protocol)

**Justification:**
- Minimal latency (no automatic retransmission)
- No head-of-line blocking
- Suitable for real-time data where recent states are more relevant than old ones

**Default Port:** 8080 (configurable)

### 2.2 Topology

```
        Client 1 ──┐
        Client 2 ──┼──> Server (authoritative)
        Client 3 ──┤
        Client 4 ──┘
```

**Communication:**
- Client → Server: Inputs, commands
- Server → Clients: Game state, events

### 2.3 Frequencies

| Data Type             | Frequency | Direction        |
|-----------------------|-----------|------------------|
| Player inputs         | Variable  | Client → Server  |
| State snapshots       | 20 Hz     | Server → Client  |
| Score update          | 20 Hz     | Server → Client  |
| Ping/Pong             | 1 Hz      | Bidirectional    |

---

## 3. Packet Format

### 3.1 General Structure

All packets follow this format:

```
+------------------+
|  PacketHeader    |  12 bytes
+------------------+
|  Payload         |  N bytes (variable depending on type)
+------------------+
```

### 3.2 Packet Header

```c
struct PacketHeader {
    uint32_t magic;            // 0x52545950 ('RTYP' in ASCII)
    PacketType type;           // 1 byte (enum)
    uint16_t payload_size;     // Payload size in bytes
    uint32_t sequence_number;  // Sequence number
} __attribute__((packed));
```

**Total size:** 12 bytes

#### 3.2.1 `magic` Field

**Type:** `uint32_t`  
**Value:** `0x52545950`  
**Order:** Big-endian

The magic number MUST be present in all packets and MUST be exactly `0x52545950`. Any packet with a different magic number MUST be rejected.

#### 3.2.2 `type` Field

**Type:** `enum PacketType : uint8_t`

Identifies the packet type (see section 4).

#### 3.2.3 `payload_size` Field

**Type:** `uint16_t`  
**Value:** Payload size in bytes (excluding header)

The server MUST verify that:
```
sizeof(PacketHeader) + payload_size == received_size
```

#### 3.2.4 `sequence_number` Field

**Type:** `uint32_t`

Monotonically increasing sequence number used for:
- Lost packet detection (optional on client side)
- Ping/pong correlation
- Debugging and logging

### 3.3 `__attribute__((packed))` Attribute

**CRITICAL:** All packet structures MUST use `__attribute__((packed))` to guarantee:
- No padding between fields
- Identical layout on all platforms (x86, ARM, Linux, Windows)
- Predictable and minimal size

**Without `packed`:**
```
struct Example {
    uint8_t  a;  // 1 byte
    // 3 bytes of padding added by compiler!
    uint32_t b;  // 4 bytes
};  // Size = 8 bytes instead of 5
```

**With `packed`:**
```
struct Example {
    uint8_t  a;  // 1 byte
    uint32_t b;  // 4 bytes
} __attribute__((packed));  // Size = 5 bytes
```

---

## 4. Packet Types

### 4.1 `PacketType` Enumeration

```c
enum class PacketType : uint8_t {
    // Connection (0x01 - 0x0F)
    CONNECT_REQUEST  = 0x01,
    CONNECT_RESPONSE = 0x02,
    DISCONNECT       = 0x03,
    PLAYER_JOINED    = 0x04,
    READY_TO_PLAY    = 0x05,
    LOBBY_STATUS     = 0x06,

    // Gameplay (0x10 - 0x1F)
    PLAYER_INPUT     = 0x10,
    PLAYER_MOVE      = 0x11,
    PLAYER_SHOOT     = 0x12,

    // World state (0x20 - 0x2F)
    WORLD_STATE      = 0x20,
    ENTITY_SPAWN     = 0x21,
    ENTITY_DESTROY   = 0x22,
    ENTITY_UPDATE    = 0x23,
    SCORE_UPDATE     = 0x24,

    // Game management (0x30 - 0x3F)
    GAME_ON          = 0x30,
    GAME_OFF         = 0x31,
    GAME_OVER        = 0x32,

    // System (0xF0 - 0xFF)
    PING             = 0xF0,
    PONG             = 0xF1
};
```

**Organization:**
- `0x01-0x0F`: Connection and lobby
- `0x10-0x1F`: Player inputs and actions
- `0x20-0x2F`: State synchronization
- `0x30-0x3F`: Game lifecycle
- `0xF0-0xFF`: Maintenance and diagnostics

---

### 4.2 Connection Packets

#### 4.2.1 CONNECT_REQUEST (0x01)

**Direction:** Client → Server  
**Purpose:** Request connection to server

```c
struct ConnectRequest {
    PacketHeader header;
    char client_version[16];  // Client version (null-terminated)
    char player_name[32];     // Player name (null-terminated)
} __attribute__((packed));
```

**Total size:** 12 + 16 + 32 = 60 bytes

**Fields:**
- `client_version`: Client version in "X.Y.Z" format (e.g., "1.0.0")
- `player_name`: Player display name (max 31 characters + '\0')

**Expected behavior:**
1. Client sends this packet to initiate connection
2. Server responds with `CONNECT_RESPONSE`
3. If rejected, client MUST NOT retry without user intervention

---

#### 4.2.2 CONNECT_RESPONSE (0x02)

**Direction:** Server → Client  
**Purpose:** Response to connection request

```c
enum class ConnectionStatus : uint8_t {
    ACCEPTED          = 0,
    REJECTED_FULL     = 1,  // Server full (4 players max)
    REJECTED_VERSION  = 2,  // Incompatible version
    REJECTED_BANNED   = 3   // Client banned (reserved)
};

struct ConnectResponse {
    PacketHeader header;
    ConnectionStatus status;
    uint32_t client_id;
    uint8_t assigned_player_slot;
    char server_version[16];
} __attribute__((packed));
```

**Total size:** 12 + 1 + 4 + 1 + 16 = 34 bytes

**Fields:**
- `status`: Connection request result
- `client_id`: Unique assigned ID (valid only if `status == ACCEPTED`)
- `assigned_player_slot`: Slot from 0 to 3 (determines spawn position)
- `server_version`: Server version (for display)

**Behavior:**
- If `ACCEPTED`: client stores `client_id` and `assigned_player_slot`
- If `REJECTED_*`: client displays appropriate error message

---

#### 4.2.3 DISCONNECT (0x03)

**Direction:** Bidirectional  
**Purpose:** Notify disconnection (graceful or timeout)

```c
struct DisconnectPacket {
    PacketHeader header;
    uint32_t client_id;
    uint8_t reason;
} __attribute__((packed));
```

**Total size:** 12 + 4 + 1 = 17 bytes

**Fields:**
- `client_id`: ID of disconnecting client
- `reason`: Reason code
  - `0`: Voluntary disconnection
  - `1`: Timeout (no response)
  - `2+`: Reserved for future extensions

**Flows:**

**Client disconnection:**
```
Client → Server: DISCONNECT (reason=0, my client_id)
Server → Others: DISCONNECT (reason=0, left client_id)
```

**Server timeout:**
```
Server → All: DISCONNECT (reason=1, timed out client_id)
```

---

#### 4.2.4 READY_TO_PLAY (0x05)

**Direction:** Client → Server  
**Purpose:** Signal that player is ready to start

```c
struct ReadyToPlay {
    PacketHeader header;
    uint32_t client_id;
    uint8_t ready;  // 0 = not ready, 1 = ready
} __attribute__((packed));
```

**Total size:** 12 + 4 + 1 = 17 bytes

**Behavior:**
1. Client sends `ready=1` when player clicks "Ready"
2. Server updates status and broadcasts `LOBBY_STATUS`
3. If all players are ready, server sends `GAME_ON`

---

#### 4.2.5 LOBBY_STATUS (0x06)

**Direction:** Server → Clients  
**Purpose:** Synchronize lobby state

```c
struct LobbyStatus {
    PacketHeader header;
    uint8_t max_players;
    uint8_t players_connected;
    uint8_t players_ready;
    uint8_t ready_mask;
    uint8_t game_started;
} __attribute__((packed));
```

**Total size:** 12 + 5 = 17 bytes

**Fields:**
- `max_players`: Maximum capacity (always 4)
- `players_connected`: Number of currently connected players
- `players_ready`: Number of players marked "ready"
- `ready_mask`: Binary mask of ready slots
  ```
  Bit 0 = Slot 0 ready
  Bit 1 = Slot 1 ready
  Bit 2 = Slot 2 ready
  Bit 3 = Slot 3 ready
  
  Example: 0b00000101 = Slots 0 and 2 ready
  ```
- `game_started`: 0 = lobby, 1 = game in progress

**Send frequency:**
- On every state change (connection, disconnection, ready)
- Broadcast to all connected clients

---

### 4.3 Gameplay Packets

#### 4.3.1 PLAYER_INPUT (0x10)

**Direction:** Client → Server  
**Purpose:** Transmit player inputs in real-time

```c
struct PlayerInput {
    PacketHeader header;
    uint32_t client_id;
    uint8_t input_flags;
    uint32_t timestamp;
} __attribute__((packed));
```

**Total size:** 12 + 4 + 1 + 4 = 21 bytes

**Fields:**
- `client_id`: Sender client ID
- `input_flags`: Binary mask of pressed keys
  ```c
  enum InputFlags : uint8_t {
      MOVE_UP    = 1 << 0,  // 0b00000001
      MOVE_DOWN  = 1 << 1,  // 0b00000010
      MOVE_LEFT  = 1 << 2,  // 0b00000100
      MOVE_RIGHT = 1 << 3,  // 0b00001000
      SHOOT      = 1 << 4,  // 0b00010000
      // Bits 5-7 reserved
  };
  ```
- `timestamp`: Client timestamp (ms since game start)

**Example:**
```
Player goes up-right and shoots:
input_flags = MOVE_UP | MOVE_RIGHT | SHOOT = 0b00010101 = 0x15
```

**Frequency:**
- Sent on every input change (key press/release)
- Or at regular interval if inputs held (e.g., 60 Hz)

**Server processing:**
1. Validate `client_id`
2. Apply inputs to player entity
3. Calculate new position/state
4. Result visible in next snapshot (< 50ms)

---

### 4.4 World State Packets

#### 4.4.1 ENTITY_UPDATE (0x23)

**Direction:** Server → Clients  
**Purpose:** Synchronize entity state

```c
struct EntityUpdate {
    PacketHeader header;
    uint32_t entity_id;
    float pos_x;
    float pos_y;
    float vel_x;
    float vel_y;
    uint8_t entity_type;
    float hp_current;
    float hp_max;
    uint8_t player_slot;
} __attribute__((packed));
```

**Total size:** 12 + 4 + 4*5 + 1 + 1 = 38 bytes

**Fields:**
- `entity_id`: Unique entity ID (player, enemy, projectile)
- `pos_x`, `pos_y`: Absolute position in world
- `vel_x`, `vel_y`: Velocity (pixels/second)
- `entity_type`: Entity type
  ```c
  enum EntityType : uint8_t {
      PLAYER     = 0,
      ENEMY      = 1,
      PROJECTILE = 2,
      POWERUP    = 3
  };
  ```
- `hp_current`: Current hit points
- `hp_max`: Maximum hit points
- `player_slot`: If player entity, its slot (0-3), else 255

**Frequency:** 20 Hz (50ms between each snapshot)

**Client behavior:**
1. Receives `EntityUpdate`
2. If entity doesn't exist locally → create entity
3. If exists → update position, HP, velocity
4. Interpolate between old and new state for smooth rendering

**Interpolation (example):**
```cpp
// Rendering at 60 FPS, snapshots at 20 Hz
float t = (time_since_last_snapshot / 50ms);  // 0.0 to 1.0
render_pos = lerp(old_pos, new_pos, t);
```

---

#### 4.4.2 SCORE_UPDATE (0x24)

**Direction:** Server → Clients  
**Purpose:** Update player score

```c
struct ScoreUpdate {
    PacketHeader header;
    uint32_t client_id;
    uint32_t score;
    uint32_t enemies_killed;
} __attribute__((packed));
```

**Total size:** 12 + 4 + 4 + 4 = 24 bytes

**Fields:**
- `client_id`: Concerned player ID
- `score`: Current total score
- `enemies_killed`: Number of enemies killed

**Frequency:** 20 Hz (sent with snapshots)

---

### 4.5 Game Management Packets

#### 4.5.1 GAME_ON (0x30)

**Direction:** Server → Clients  
**Purpose:** Signal game start

```c
struct GameOn {
    PacketHeader header;
    uint32_t client_id;  // ID of client who started (or 0 if auto)
} __attribute__((packed));
```

**Total size:** 12 + 4 = 16 bytes

**Trigger:**
- All players are ready → automatic start
- Or a player forces start (admin mode)

**Client behavior:**
1. Receives `GAME_ON`
2. Hides lobby, displays game
3. Activates inputs
4. Starts processing `EntityUpdate`

---

#### 4.5.2 GAME_OVER (0x32)

**Direction:** Server → Clients  
**Purpose:** Signal game end and display scores

```c
struct PlayerFinalScore {
    uint32_t client_id;
    char player_name[32];
    uint32_t score;
    uint32_t enemies_killed;
} __attribute__((packed));

struct GameOver {
    PacketHeader header;
    uint8_t num_players;
    PlayerFinalScore players[4];
} __attribute__((packed));
```

**Total size:** 12 + 1 + (4+32+4+4)*4 = 12 + 1 + 176 = 189 bytes

**Fields:**
- `num_players`: Number of players in ranking (1-4)
- `players`: Array sorted by descending score

**Trigger:**
- All players are dead
- Game timer expired (if implemented)

**Client behavior:**
1. Receives `GAME_OVER`
2. Displays score screen
3. Returns to lobby after timeout or user action

---

### 4.6 System Packets

#### 4.6.1 PING (0xF0)

**Direction:** Bidirectional  
**Purpose:** Measure latency and maintain connection

```c
struct PacketHeader {
    // No additional payload
} __attribute__((packed));
```

**Total size:** 12 bytes (header only)

**Flow:**
```
Client → Server: PING (sequence=123)
Server → Client: PONG (sequence=123)
Client calculates: RTT = response_time
```

**Recommended frequency:** 1 Hz (every second)

**Uses:**
- Latency measurement (Round-Trip Time)
- Dead connection detection
- Keep session alive (avoids NAT timeouts)

#### 4.6.2 PONG (0xF1)

**Direction:** Response to PING  
**Purpose:** Acknowledgment

Same structure as PING, with identical `sequence_number`.

---

## 5. Communication Flows

### 5.1 Initial Connection

```
Client                                    Server
  │                                         │
  ├─────── CONNECT_REQUEST ───────────────>│
  │        (version, player_name)          │
  │                                         ├─ Validate version
  │                                         ├─ Check capacity
  │                                         ├─ Assign slot
  │<──────── CONNECT_RESPONSE ─────────────┤
  │          (ACCEPTED, client_id, slot)   │
  │                                         │
  │<──────── LOBBY_STATUS ──────────────────┤
  │          (1/4 players, 0 ready)        │
  │                                         │
```

### 5.2 Lobby Phase

```
Client 1                Server              Client 2
  │                       │                     │
  ├─ READY_TO_PLAY ─────>│                     │
  │  (ready=1)            ├─ Update            │
  │                       │                     │
  │<─ LOBBY_STATUS ───────┼──────────────────>│
  │  (1/2 ready)          │                     │
  │                       │<─ READY_TO_PLAY ───┤
  │                       │   (ready=1)         │
  │<─ LOBBY_STATUS ───────┼──────────────────>│
  │  (2/2 ready)          │                     │
  │                       │                     │
  │<─ GAME_ON ────────────┼──────────────────>│
  │                       │                     │
```

### 5.3 Game Phase

```
Client                          Server
  │                               │
  ├─ PLAYER_INPUT ───────────────>│
  │  (MOVE_UP | SHOOT)            ├─ update()
  │                               ├─ Collisions
  │                               ├─ Physics
  │<─ ENTITY_UPDATE ──────────────┤ [20 Hz]
  │  (pos, vel, hp)               │
  │<─ ENTITY_UPDATE ──────────────┤
  │  (enemy 1)                    │
  │<─ ENTITY_UPDATE ──────────────┤
  │  (enemy 2)                    │
  │<─ SCORE_UPDATE ───────────────┤
  │  (score, kills)               │
  │                               │
  ├─ PLAYER_INPUT ───────────────>│
  │  (MOVE_LEFT)                  │
  │                               │
  │         [50ms]                │
  │<─ ENTITY_UPDATE ──────────────┤
  │  (new pos)                    │
```

### 5.4 Game End

```
Client                                  Server
  │                                       │
  │                                       ├─ Detect all dead
  │<────────── GAME_OVER ────────────────┤
  │            (sorted final scores)     │
  │                                       │
  [Display scores]                       │
  │                                       │
  ├─ Auto return to lobby                │
  │<────────── LOBBY_STATUS ─────────────┤
  │            (game_started=0)          │
```

---

## 6. Error Handling

### 6.1 Invalid Packets

**Rejection cases:**
1. Incorrect magic number (`!= 0x52545950`)
2. Inconsistent size (`received_size != header + payload_size`)
3. Insufficient payload for packet type

**Server behavior:**
- Error log (source IP, reason)
- Silent rejection (no response to client)
- No impact on other connections

### 6.2 Connection Timeout

**Definition:** No packet received from a client for `CLIENT_TIMEOUT` seconds.

**Recommended value:** 10 seconds

**Behavior:**
1. Server detects timeout
2. Server sends `DISCONNECT (reason=1)` to other clients
3. Server cleans up client resources
4. If all clients timeout → game reset

**Detection mechanism:**
```cpp
for (auto& client : clients) {
    if (now - client.last_activity > CLIENT_TIMEOUT) {
        disconnect(client.id, REASON_TIMEOUT);
    }
}
```

### 6.3 Packet Loss

**UDP doesn't guarantee delivery** → packet loss possible.

**Mitigation strategies:**

1. **Frequent snapshots (20 Hz)**: a lost packet is quickly replaced
2. **Complete state**: each `EntityUpdate` contains all necessary info
3. **Client interpolation**: smooths jumps caused by losses
4. **Redundant inputs**: client can resend same input if no ack (optional)

**No automatic retransmission**: recent states are more relevant than old ones.

### 6.4 Packet Ordering

**UDP doesn't guarantee order** → packets may arrive out of order.

**Management via `sequence_number`:**
```cpp
if (packet.sequence_number > last_processed_sequence) {
    processPacket(packet);
    last_processed_sequence = packet.sequence_number;
} else {
    // Old or duplicate packet → ignored
}
```

**Note:** For R-Type, snapshot order is not critical as each snapshot is complete and autonomous.

---

## 7. Security Considerations

### 7.1 Server-Side Validation

**Principle:** Never trust the client.

**Mandatory validations:**
1. `client_id` matches source IP:port address
2. Assigned slot exists and is valid
3. Inputs are physically possible (no teleportation)
4. Cooldowns respected (shooting, dash, etc.)

### 7.2 Attack Protection

#### 7.2.1 Packet Flooding

**Attack:** Massive packet sending to saturate server.

**Mitigation:**
- Rate limiting per IP (e.g., max 100 packets/second)
- Reject packets beyond threshold
- Temporary ban of abusive IPs

#### 7.2.2 IP Spoofing

**Attack:** Source IP address impersonation.

**Mitigation:**
- IP:port consistency check for each `client_id`
- Connection state (client can't send inputs before `CONNECT_RESPONSE`)

#### 7.2.3 Packet Injection

**Attack:** Sending malformed packets to crash server.

**Mitigation:**
- Strict validation of all fields
- No unchecked casts
- Maximum packet size (`MAX_PACKET_SIZE = 1024`)

### 7.3 Confidentiality

**Current state:** No encryption (cleartext protocol).

**Implications:**
- Packets can be intercepted (sniffing)
- No sensitive data transmitted (no passwords, banking info)
- Acceptable for local/LAN multiplayer game

**Future evolution:**
- DTLS implementation (TLS over UDP) for encryption
- JWT token authentication
- Critical packet signing

---

## 8. Appendices

### 8.1 Binary Packet Examples

#### Example 1: CONNECT_REQUEST

```
Hexadecimal                          ASCII
52 54 59 50                          RTYP (magic)
01                                   CONNECT_REQUEST
00 30                                payload_size = 48
00 00 00 00                          sequence = 0
31 2E 30 2E 30 00 ... (16 bytes)    client_version = "1.0.0"
50 6C 61 79 65 72 31 00 ... (32)    player_name = "Player1"
```

**Total size:** 60 bytes

#### Example 2: PLAYER_INPUT

```
Hexadecimal                          Meaning
52 54 59 50                          RTYP (magic)
10                                   PLAYER_INPUT
00 09                                payload_size = 9
00 00 00 7B                          sequence = 123
00 00 00 01                          client_id = 1
11                                   input_flags = MOVE_UP | SHOOT
00 00 03 E8                          timestamp = 1000ms
```

**Total size:** 21 bytes

**Decoding `input_flags = 0x11`:**
```
0x11 = 0b00010001
       │      └─ Bit 0: MOVE_UP
       └──────── Bit 4: SHOOT
```

#### Example 3: ENTITY_UPDATE

```
Hexadecimal                          Meaning
52 54 59 50                          RTYP
23                                   ENTITY_UPDATE
00 1A                                payload_size = 26
00 00 01 2C                          sequence = 300
00 00 00 0A                          entity_id = 10
43 48 00 00                          pos_x = 200.0
43 96 00 00                          pos_y = 300.0
40 00 00 00                          vel_x = 2.0
00 00 00 00                          vel_y = 0.0
00                                   entity_type = PLAYER
42 C8 00 00                          hp_current = 100.0
42 C8 00 00                          hp_max = 100.0
00                                   player_slot = 0
```

**Total size:** 38 bytes

### 8.2 Bandwidth Calculation

#### Scenario: 4 players, 20 active entities

**Per snapshot (20 Hz):**
- 4 players × 38 bytes = 152 bytes
- 16 enemies × 38 bytes = 608 bytes
- 4 scores × 24 bytes = 96 bytes
- **Total per snapshot:** 856 bytes

**Per second (20 Hz):**
- 856 bytes × 20 = 17,120 bytes/s = **17.12 KB/s**

**Per player:**
- Reception: ~17 KB/s
- Sending (inputs): ~1 KB/s (assumed 50 inputs/s × 21 bytes)
- **Total bidirectional:** ~18 KB/s = **144 Kbps**

**For 4 players (server):**
- Reception: 4 KB/s
- Sending: 4 × 17 KB/s = 68 KB/s
- **Server total:** ~72 KB/s = **576 Kbps**

**Conclusion:** Very reasonable bandwidth, compatible with ADSL/4G connections.

### 8.3 Client State Diagram

```
┌──────────┐
│  INIT    │
└────┬─────┘
     │ start()
     v
┌──────────────┐  CONNECT_REQUEST
│ CONNECTING   ├─────────────────>
└──────┬───────┘
       │ CONNECT_RESPONSE (ACCEPTED)
       v
┌──────────────┐  READY_TO_PLAY
│   LOBBY      ├─────────────────>
└──────┬───────┘
       │ GAME_ON
       v
┌──────────────┐  PLAYER_INPUT (continuous)
│  IN_GAME     ├─────────────────>
└──────┬───────┘
       │ GAME_OVER
       v
┌──────────────┐
│ GAME_OVER    │
└──────┬───────┘
       │ return to lobby
       v
    [LOBBY]
```

### 8.4 Server State Diagram

```
┌──────────┐
│  IDLE    │
└────┬─────┘
     │ start()
     v
┌──────────────┐  CONNECT_REQUEST
│  ACCEPTING   │<─────────────────
└──────┬───────┘  (accept clients)
       │ hasClients()
       v
┌──────────────┐  wait for ready
│   LOBBY      │<─────────────────
└──────┬───────┘  READY_TO_PLAY
       │ allReady()
       v
┌──────────────┐  update() + broadcast()
│  RUNNING     │<─────────────────
└──────┬───────┘  PLAYER_INPUT
       │ allDead()
       v
┌──────────────┐
│  GAME_OVER   │
└──────┬───────┘
       │ timeout or noClients()
       v
    [LOBBY]
```

### 8.5 Version Compatibility

**Format:** `MAJOR.MINOR.PATCH` (e.g., "1.2.3")

**Compatibility rules:**
- **Different MAJOR**: incompatible (REJECTED_VERSION)
- **Different MINOR**: warning, but accepted (optional features)
- **Different PATCH**: compatible (bugfixes)

**Example:**
```
Client 1.2.3 → Server 1.2.5: OK
Client 1.3.0 → Server 1.2.5: OK (warning)
Client 2.0.0 → Server 1.5.0: REJECTED_VERSION
```

### 8.6 Future Extensions

**Reserved packets:**
- `0x04`: `PLAYER_JOINED` (push notification)
- `0x11`: `PLAYER_MOVE` (optimized movement)
- `0x12`: `PLAYER_SHOOT` (optimized shooting)
- `0x20`: `WORLD_STATE` (compressed full snapshot)
- `0x21`: `ENTITY_SPAWN` (spawn event)
- `0x22`: `ENTITY_DESTROY` (destruction event)
- `0x31`: `GAME_OFF` (pause)

**Planned features:**
- Snapshot compression (zlib)
- Client-side prediction
- Lag compensation
- Spectator mode
- Replay recording

---

## 9. Compliance and References

### 9.1 Used Standards

- **RFC 768**: User Datagram Protocol (UDP)
- **RFC 2119**: Key words for use in RFCs (MUST, SHOULD, MAY)
- **IEEE 754**: Floating-point arithmetic (for `float`)


### 9.2 Reference Implementations

- **Server**: `src/server/Server.cpp` (C++)
- **Client**: `src/game/NetworkClient.cpp` (C++)
- **Protocol**: `include/Protocol.hpp` (packed structures)

---

## 10. Changelog

### Version 1.0 (Initial Release)
- Complete base protocol specification
- Packet types 0x01-0x32, 0xF0-0xF1
- Communication flow documentation
- Binary examples and bandwidth calculations

---

**End of RFC document**

This protocol is subject to evolution. Any modification MUST increment the version number and document changes in the changelog.