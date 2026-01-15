#pragma once

#include "engine/core/Component.hpp"

class EnemyShooter : public Component {
public:
    EnemyShooter();
    EnemyShooter(float shootInterval, float bulletSpeed);
    ~EnemyShooter() override = default;

    float shootInterval;
    float currentCooldown;
    float bulletSpeed; 
};