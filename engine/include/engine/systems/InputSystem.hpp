#pragma once

#include "engine/core/System.hpp"

class InputSystem : public System {
public:
    InputSystem();

    void update(Registry& registry, float deltaTime) override;
};