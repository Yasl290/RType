#pragma once

#include "engine/core/System.hpp"

class ServerShootingSystem : public System {
public:
    explicit ServerShootingSystem(float windowWidth);

    void update(Registry& registry, float deltaTime) override;

private:
    float _windowWidth;
    float _chargeTime;
    bool _wasShooting;

    void createNormalShot(Registry& registry, float x, float y, float damage = 10.f);
    void createChargedShot(Registry& registry, float x, float y, float damage = 50.f);
};