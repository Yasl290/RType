#pragma once

#include "engine/core/System.hpp"

class AnimationSystem : public System {
public:
    AnimationSystem();

    void update(Registry& registry, float deltaTime) override;
};