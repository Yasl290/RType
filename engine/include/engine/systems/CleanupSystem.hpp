#pragma once

#include "engine/core/System.hpp"

class CleanupSystem : public System {
public:
    explicit CleanupSystem(float windowWidth);

    void update(Registry& registry, float deltaTime) override;

private:
    float _windowWidth;
};