#pragma once
#include <cstdint>
#include <cstring>

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

    GAME_ON = 0x30,
    GAME_OFF = 0x31,
    GAME_OVER = 0x32,

    PING = 0xF0,
    PONG = 0xF1
};

struct PacketHeader {
    uint32_t magic;
    PacketType type;
    uint16_t payload_size;
    uint32_t sequence_number;

    PacketHeader() : magic(0x52545950), type(PacketType::PING),
                     payload_size(0), sequence_number(0) {}
} __attribute__((packed));


struct ConnectRequest {
    PacketHeader header;
    char client_version[16];
    char player_name[32];
    
    ConnectRequest() {
        header.type = PacketType::CONNECT_REQUEST;
        header.payload_size = sizeof(ConnectRequest) - sizeof(PacketHeader);
        std::memset(client_version, 0, sizeof(client_version));
        std::memset(player_name, 0, sizeof(player_name));
    }
} __attribute__((packed));

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
        header.payload_size = sizeof(ConnectResponse) - sizeof(PacketHeader);
        std::memset(server_version, 0, sizeof(server_version));
    }
} __attribute__((packed));

struct DisconnectPacket {
    PacketHeader header;
    uint32_t client_id;
    uint8_t reason;
    
    DisconnectPacket() : client_id(0), reason(0) {
        header.type = PacketType::DISCONNECT;
        header.payload_size = sizeof(DisconnectPacket) - sizeof(PacketHeader);
    }
} __attribute__((packed));


struct PlayerInput {
    PacketHeader header;
    uint32_t client_id;
    uint8_t input_flags;
    uint32_t timestamp;
    
    PlayerInput() : client_id(0), input_flags(0), timestamp(0) {
        header.type = PacketType::PLAYER_INPUT;
        header.payload_size = sizeof(PlayerInput) - sizeof(PacketHeader);
    }
} __attribute__((packed));


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
        header.payload_size = sizeof(EntityUpdate) - sizeof(PacketHeader);
    }
} __attribute__((packed));

struct EntitySpawn {
    PacketHeader header;
    uint32_t entity_id;
    uint8_t entity_type;
    float pos_x;
    float pos_y;
    
    EntitySpawn() : entity_id(0), entity_type(0), pos_x(0), pos_y(0) {
        header.type = PacketType::ENTITY_SPAWN;
        header.payload_size = sizeof(EntitySpawn) - sizeof(PacketHeader);
    }
} __attribute__((packed));

struct EntityDestroy {
    PacketHeader header;
    uint32_t entity_id;
    uint8_t reason;
    uint32_t killer_id;
    
    EntityDestroy() : entity_id(0), reason(0), killer_id(0) {
        header.type = PacketType::ENTITY_DESTROY;
        header.payload_size = sizeof(EntityDestroy) - sizeof(PacketHeader);
    }
} __attribute__((packed));

struct EntityFire {
    PacketHeader header;
    uint32_t shooter_id;
    uint32_t projectile_id;
    float pos_x;
    float pos_y;
    uint8_t projectile_type;
    
    EntityFire() : shooter_id(0), projectile_id(0), pos_x(0), pos_y(0), projectile_type(0) {
        header.type = PacketType::ENTITY_FIRE;
        header.payload_size = sizeof(EntityFire) - sizeof(PacketHeader);
    }
} __attribute__((packed));

struct PlayerDeath {
    PacketHeader header;
    uint32_t player_id;
    uint32_t killer_id;
    uint8_t death_type;
    
    PlayerDeath() : player_id(0), killer_id(0), death_type(0) {
        header.type = PacketType::PLAYER_DEATH;
        header.payload_size = sizeof(PlayerDeath) - sizeof(PacketHeader);
    }
} __attribute__((packed));

struct GameOn {
    PacketHeader header;
    uint32_t client_id;
    
    GameOn() : client_id(0) {
        header.type = PacketType::GAME_ON;
        header.payload_size = sizeof(GameOn) - sizeof(PacketHeader);
    }
} __attribute__((packed));

struct ReadyToPlay {
    PacketHeader header;
    uint32_t client_id;
    uint8_t ready;

    ReadyToPlay() : client_id(0), ready(0) {
        header.type = PacketType::READY_TO_PLAY;
        header.payload_size = sizeof(ReadyToPlay) - sizeof(PacketHeader);
    }
} __attribute__((packed));

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
        header.payload_size = sizeof(LobbyStatus) - sizeof(PacketHeader);
    }
} __attribute__((packed));

struct ScoreUpdate {
    PacketHeader header;
    uint32_t client_id;
    uint32_t score;
    uint32_t enemies_killed;
    
    ScoreUpdate() : client_id(0), score(0), enemies_killed(0) {
        header.type = PacketType::SCORE_UPDATE;
        header.payload_size = sizeof(ScoreUpdate) - sizeof(PacketHeader);
    }
} __attribute__((packed));

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
} __attribute__((packed));

struct GameOver {
    PacketHeader header;
    uint8_t num_players;
    PlayerFinalScore players[4];
    
    GameOver() : num_players(0), players{} {
        header.type = PacketType::GAME_OVER;
        header.payload_size = sizeof(GameOver) - sizeof(PacketHeader);
    }
} __attribute__((packed));

inline bool IsValidPacket(const PacketHeader& header) { return header.magic == 0x52545950; }

} // namespace RType::Protocol