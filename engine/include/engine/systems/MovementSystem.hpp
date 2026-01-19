#pragma once

#include "engine/core/System.hpp"

class MovementSystem : public System {
public:
    MovementSystem();

    void update(Registry& registry, float deltaTime) override;
};