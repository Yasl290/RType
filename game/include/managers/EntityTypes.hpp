#pragma once
#include <cstdint>

namespace RType {

enum class EntityType : uint8_t {
    PLAYER = 0,
    ENEMY_BASIC = 1,
    PROJECTILE_NORMAL = 2,
    PROJECTILE_CHARGED = 3,
    PROJECTILE_ENEMY = 4,
    BOSS = 5,
    ENEMY_FASTSHOOTER = 6,
    ENEMY_BOMBER = 7
};

inline EntityType toEntityType(uint8_t value) {
    return static_cast<EntityType>(value);
}

inline uint8_t fromEntityType(EntityType type) {
    return static_cast<uint8_t>(type);
}

enum class InputFlag : uint8_t {
    NONE = 0x00,
    UP = 0x01,
    DOWN = 0x02,
    LEFT = 0x04,
    RIGHT = 0x08,
    SHOOT = 0x10
};

inline InputFlag operator|(InputFlag a, InputFlag b) {
    return static_cast<InputFlag>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
    );
}

inline InputFlag operator&(InputFlag a, InputFlag b) {
    return static_cast<InputFlag>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
    );
}

inline uint8_t operator|(uint8_t a, InputFlag b) {
    return a | static_cast<uint8_t>(b);
}

inline uint8_t operator&(uint8_t a, InputFlag b) {
    return a & static_cast<uint8_t>(b);
}

inline bool hasFlag(uint8_t flags, InputFlag flag) {
    return (flags & static_cast<uint8_t>(flag)) != 0;
}

enum class DeathCause : uint8_t {
    OUT_OF_BOUNDS = 0,
    ENEMY_CONTACT = 1,
    ENEMY_PROJECTILE = 2
};

}