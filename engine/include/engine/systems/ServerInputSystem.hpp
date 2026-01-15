#pragma once

#include "engine/core/System.hpp"

class ServerInputSystem : public System {
public:
    ServerInputSystem();

    void update(Registry& registry, float deltaTime) override;
};