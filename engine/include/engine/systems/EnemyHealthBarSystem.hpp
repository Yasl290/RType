#pragma once

#include "engine/core/System.hpp"


class EnemyHealthBarSystem : public System {
public:
    EnemyHealthBarSystem();

    void update(Registry& registry, float deltaTime) override;
    void render(Registry& registry, Renderer& renderer) override;
};