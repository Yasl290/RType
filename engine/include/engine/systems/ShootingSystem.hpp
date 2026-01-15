#pragma once

#include "engine/core/System.hpp"

class ShootingSystem : public System {
public:
    explicit ShootingSystem(float windowWidth);

    void update(Registry& registry, float deltaTime) override;

private:
    void createNormalShot(Registry& registry, float x, float y);
    void createChargedShot(Registry& registry, float x, float y);

    float _windowWidth;
    float _chargeTime;
    bool _wasSpacePressed;
};