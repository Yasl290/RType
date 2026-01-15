#pragma once

#include "engine/core/System.hpp"

class EnemyShootingSystem : public System {
public:
    EnemyShootingSystem();

    void update(Registry& registry, float deltaTime) override;
};