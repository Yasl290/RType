#pragma once

#include "engine/core/System.hpp"

class CollisionSystem : public System {
public:
    CollisionSystem();

    void update(Registry& registry, float deltaTime) override;
};