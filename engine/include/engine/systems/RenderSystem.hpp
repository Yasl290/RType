#pragma once

#include "engine/core/System.hpp"

class RenderSystem : public System {
public:
    RenderSystem();

    void update(Registry& registry, float deltaTime) override;
    void render(Registry& registry, Renderer& renderer) override;
};