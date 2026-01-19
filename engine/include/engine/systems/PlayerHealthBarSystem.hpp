#pragma once

#include "engine/core/System.hpp"

class PlayerHealthBarSystem : public System {
public:
    PlayerHealthBarSystem();

    void update(Registry& registry, float deltaTime) override;
    void render(Registry& registry, Renderer& renderer) override;
};
