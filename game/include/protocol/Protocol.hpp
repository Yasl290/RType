#pragma once
#include <cstdint>
#include <cstring>

#ifdef _MSC_VER
    #pragma pack(push, 1)
    #define PACKED
#else
    #define PACKED __attribute__((packed))
#endif

namespace RType::Protocol {

enum class PacketType : uint8_t {
    CONNECT_REQUEST = 0x01,
    CONNECT_RESPONSE = 0x02,
    DISCONNECT = 0x03,
    PLAYER_JOINED = 0x04,
    READY_TO_PLAY = 0x05,
    LOBBY_STATUS = 0x06,
    PLAYER_INPUT = 0x10,
    PLAYER_MOVE = 0x11,
    PLAYER_SHOOT = 0x12,
    WORLD_STATE = 0x20,
    ENTITY_SPAWN = 0x21,
    ENTITY_DESTROY = 0x22,
    ENTITY_UPDATE = 0x23,
    SCORE_UPDATE = 0x24,
    ENTITY_FIRE = 0x25,
    PLAYER_DEATH = 0x26,
    GAME_EVENT = 0x27,
    GAME_ON = 0x30,
    GAME_OFF = 0x31,
    GAME_OVER = 0x32,
    ACK = 0xE0,
    PING = 0xF0,
    PONG = 0xF1
};

enum class ReliabilityFlag : uint8_t {
    UNRELIABLE = 0x00,
    RELIABLE = 0x80
};

enum class EntityType : uint8_t {
    PLAYER = 0,
    ENEMY_BASIC = 1,
    PROJECTILE_NORMAL = 2,
    PROJECTILE_CHARGED = 3,
    PROJECTILE_ENEMY = 4,
    BOSS = 5,
    ENEMY_FASTSHOOTER = 6,  // ✓ Ajoutez
    ENEMY_BOMBER = 7   
};

enum class GameEventType : uint8_t {
    PLAYER_DEATH = 0,
    ENEMY_KILLED = 1,
    BOSS_KILLED = 2,
    GAME_OVER = 3,
    LEVEL_COMPLETE = 4
};

inline uint8_t toEntityTypeId(EntityType type) {
    return static_cast<uint8_t>(type);
}

inline EntityType fromEntityTypeId(uint8_t value) {
    return static_cast<EntityType>(value);
}

struct PacketHeader {
    uint16_t magic;
    PacketType type;
    uint16_t sequence_number;
    uint8_t flags;

    PacketHeader() : magic(0xBEEF), type(PacketType::PING),
                     sequence_number(0), flags(0) {}
                     
    bool isReliable() const { 
        return (flags & static_cast<uint8_t>(ReliabilityFlag::RELIABLE)) != 0; 
    }
    
    void setReliable(bool reliable) {
        if (reliable) {
            flags |= static_cast<uint8_t>(ReliabilityFlag::RELIABLE);
        } else {
            flags &= ~static_cast<uint8_t>(ReliabilityFlag::RELIABLE);
        }
    }
} PACKED;

struct AckPacket {
    PacketHeader header;
    uint16_t ack_sequence;
    
    AckPacket() : ack_sequence(0) {
        header.type = PacketType::ACK;
    }
} PACKED;

struct ConnectRequest {
    PacketHeader header;
    char client_version[16];
    char player_name[32];
    
    ConnectRequest() {
        header.type = PacketType::CONNECT_REQUEST;
        header.setReliable(false);
        std::memset(client_version, 0, sizeof(client_version));
        std::memset(player_name, 0, sizeof(player_name));
    }
} PACKED;

enum class ConnectionStatus : uint8_t {
    ACCEPTED = 0,
    REJECTED_FULL = 1,
    REJECTED_VERSION = 2,
    REJECTED_BANNED = 3
};

struct ConnectResponse {
    PacketHeader header;
    ConnectionStatus status;
    uint32_t client_id;
    uint8_t assigned_player_slot;
    char server_version[16];
    
    ConnectResponse() : status(ConnectionStatus::ACCEPTED), 
                        client_id(0), assigned_player_slot(0) {
        header.type = PacketType::CONNECT_RESPONSE;
        header.setReliable(false);
        std::memset(server_version, 0, sizeof(server_version));
    }
} PACKED;

struct DisconnectPacket {
    PacketHeader header;
    uint32_t client_id;
    uint8_t reason;
    
    DisconnectPacket() : client_id(0), reason(0) {
        header.type = PacketType::DISCONNECT;
        header.setReliable(true);
    }
} PACKED;

struct PlayerInput {
    PacketHeader header;
    uint32_t client_id;
    uint8_t input_flags;
    uint32_t timestamp;
    
    PlayerInput() : client_id(0), input_flags(0), timestamp(0) {
        header.type = PacketType::PLAYER_INPUT;
    }
} PACKED;

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
    
    EntityUpdate() : entity_id(0), pos_x(0), pos_y(0), 
                     vel_x(0), vel_y(0), entity_type(0),
                     hp_current(0.f), hp_max(0.f), player_slot(255) {
        header.type = PacketType::ENTITY_UPDATE;
    }
} PACKED;

struct EntitySpawn {
    PacketHeader header;
    uint32_t entity_id;
    uint8_t entity_type;
    float pos_x;
    float pos_y;
    
    EntitySpawn() : entity_id(0), entity_type(0), pos_x(0), pos_y(0) {
        header.type = PacketType::ENTITY_SPAWN;
    }
} PACKED;

struct EntityDestroy {
    PacketHeader header;
    uint32_t entity_id;
    uint8_t reason;
    uint32_t killer_id;
    
    EntityDestroy() : entity_id(0), reason(0), killer_id(0) {
        header.type = PacketType::ENTITY_DESTROY;
        header.setReliable(true);
    }
} PACKED;

struct EntityFire {
    PacketHeader header;
    uint32_t shooter_id;
    uint32_t projectile_id;
    float pos_x;
    float pos_y;
    uint8_t projectile_type;
    
    EntityFire() : shooter_id(0), projectile_id(0), pos_x(0), pos_y(0), projectile_type(0) {
        header.type = PacketType::ENTITY_FIRE;
    }
} PACKED;

struct PlayerDeath {
    PacketHeader header;
    uint32_t player_id;
    uint32_t killer_id;
    uint8_t death_type;
    
    PlayerDeath() : player_id(0), killer_id(0), death_type(0) {
        header.type = PacketType::PLAYER_DEATH;
        header.setReliable(true);
    }
} PACKED;

struct GameEvent {
    PacketHeader header;
    GameEventType event_type;
    uint32_t entityId;
    uint32_t killerId;
    uint32_t scoreGain;
    char levelName[64];
    char nextLevelName[64];
    
    GameEvent() : event_type(GameEventType::ENEMY_KILLED), 
                  entityId(0), killerId(0), scoreGain(0) {
        header.type = PacketType::GAME_EVENT;
        header.setReliable(true);
        std::memset(levelName, 0, sizeof(levelName));
        std::memset(nextLevelName, 0, sizeof(nextLevelName));
    }
} PACKED;

struct GameOn {
    PacketHeader header;
    uint32_t client_id;
    
    GameOn() : client_id(0) {
        header.type = PacketType::GAME_ON;
    }
} PACKED;

struct ReadyToPlay {
    PacketHeader header;
    uint32_t client_id;
    uint8_t ready;

    ReadyToPlay() : client_id(0), ready(0) {
        header.type = PacketType::READY_TO_PLAY;
    }
} PACKED;

struct LobbyStatus {
    PacketHeader header;
    uint8_t max_players;
    uint8_t players_connected;
    uint8_t players_ready;
    uint8_t ready_mask;
    uint8_t game_started;

    LobbyStatus()
        : max_players(4), players_connected(0), players_ready(0), ready_mask(0), game_started(0)
    {
        header.type = PacketType::LOBBY_STATUS;
    }
} PACKED;

struct ScoreUpdate {
    PacketHeader header;
    uint32_t client_id;
    uint32_t score;
    uint32_t enemies_killed;
    
    ScoreUpdate() : client_id(0), score(0), enemies_killed(0) {
        header.type = PacketType::SCORE_UPDATE;
    }
} PACKED;

struct PlayerFinalScore {
    uint32_t client_id;
    char player_name[32];
    uint32_t score;
    uint32_t enemies_killed;
    
    PlayerFinalScore() 
        : client_id(0), score(0), enemies_killed(0) 
    {
        std::memset(player_name, 0, sizeof(player_name));
    }
} PACKED;

struct GameOver {
    PacketHeader header;
    uint8_t num_players;
    PlayerFinalScore players[4];

    GameOver() : num_players(0), players{} {
        header.type = PacketType::GAME_OVER;
        header.setReliable(true);
    }
} PACKED;

// ============================================================================
// ADVANCED NETWORKING FEATURES - Track #2
// ============================================================================

// Quantization helpers - Convert float to int16 with 0.1 precision
inline int16_t quantizePosition(float value) {
    return static_cast<int16_t>(value * 10.0f);
}

inline int16_t quantizeVelocity(float value) {
    return static_cast<int16_t>(value * 10.0f);
}

inline int16_t quantizeHP(float value) {
    // Use 1.0 precision for HP to avoid overflow (Boss has 10000 HP)
    // int16 max = 32767, so we can store HP up to 32767
    return static_cast<int16_t>(value);
}

inline float dequantizePosition(int16_t value) {
    return static_cast<float>(value) / 10.0f;
}

inline float dequantizeVelocity(int16_t value) {
    return static_cast<float>(value) / 10.0f;
}

inline float dequantizeHP(int16_t value) {
    // HP has 1.0 precision (no decimal)
    return static_cast<float>(value);
}

// Delta compression change mask bits
enum EntityChangeMask : uint8_t {
    CHANGED_POSITION    = 0x01,  // pos_x, pos_y changed
    CHANGED_VELOCITY    = 0x02,  // vel_x, vel_y changed
    CHANGED_HP          = 0x04,  // hp_current, hp_max changed
    CHANGED_TYPE        = 0x08,  // entity_type changed (rare)
    CHANGED_SLOT        = 0x10   // player_slot changed (rare)
};

// Quantized entity data (float 32bit -> int16)
// Reduces bandwidth by ~50% on position/velocity/HP
struct QuantizedEntityData {
    uint32_t entity_id;
    int16_t pos_x;          // 4 bytes -> 2 bytes
    int16_t pos_y;          // 4 bytes -> 2 bytes
    int16_t vel_x;          // 4 bytes -> 2 bytes
    int16_t vel_y;          // 4 bytes -> 2 bytes
    int16_t hp_current;     // 4 bytes -> 2 bytes
    int16_t hp_max;         // 4 bytes -> 2 bytes
    uint8_t entity_type;
    uint8_t player_slot;

    QuantizedEntityData()
        : entity_id(0), pos_x(0), pos_y(0), vel_x(0), vel_y(0),
          hp_current(0), hp_max(0), entity_type(0), player_slot(255) {}
} PACKED;

// Batched entity update - Pack multiple entities in ONE packet
// Reduces packet overhead by ~98% (50 packets -> 1 packet)
struct BatchedEntityUpdate {
    PacketHeader header;
    uint16_t entity_count;  // Number of entities in this batch
    QuantizedEntityData entities[64];  // Max 64 entities per packet

    BatchedEntityUpdate() : entity_count(0), entities{} {
        header.type = PacketType::ENTITY_UPDATE;
    }

    size_t getPacketSize() const {
        return sizeof(PacketHeader) + sizeof(uint16_t) +
               (entity_count * sizeof(QuantizedEntityData));
    }
} PACKED;

// Delta compressed entity - Only send changed fields
// Reduces bandwidth by ~70% by sending only what changed
struct EntityDeltaData {
    uint32_t entity_id;
    uint8_t change_mask;    // Bitmask of what changed (EntityChangeMask)

    // Optional fields (only present if corresponding bit in change_mask is set)
    int16_t pos_x;
    int16_t pos_y;
    int16_t vel_x;
    int16_t vel_y;
    int16_t hp_current;
    int16_t hp_max;
    uint8_t entity_type;
    uint8_t player_slot;

    EntityDeltaData()
        : entity_id(0), change_mask(0), pos_x(0), pos_y(0),
          vel_x(0), vel_y(0), hp_current(0), hp_max(0),
          entity_type(0), player_slot(255) {}
} PACKED;

inline bool IsValidPacket(const PacketHeader& header) {
    return header.magic == 0xBEEF;
}

}

#ifdef _MSC_VER
    #pragma pack(pop)
#endif