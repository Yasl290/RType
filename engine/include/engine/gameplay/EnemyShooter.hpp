#pragma once

#include "engine/core/Component.hpp"
#include "engine/gameplay/Enemy.hpp"

class EnemyShooter : public Component {
public:
    EnemyShooter();
    EnemyShooter(float shootInterval, float bulletSpeed);
    ~EnemyShooter() override = default;

    static EnemyShooter createForType(EnemyType type);

    float shootInterval;
    float currentCooldown;
    float bulletSpeed; 
};