#pragma once

#include "engine/core/Component.hpp"
#include <cstdint>

enum class ProjectileType {
    Normal,
    Charged
};

class Projectile : public Component {
public:
    Projectile();
    Projectile(ProjectileType type, float damage, bool piercing, uint32_t ownerId = 0);
    ~Projectile() override = default;

    ProjectileType type;
    float damage;
    bool piercing;
    bool isPlayerProjectile;
    uint32_t ownerId;
};