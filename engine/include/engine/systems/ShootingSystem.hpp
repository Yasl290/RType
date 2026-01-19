#pragma once

#include "engine/core/System.hpp"
#include "engine/core/Registry.hpp"

class ShootingSystem : public System {
public:
    explicit ShootingSystem(float windowWidth);
    
    void update(Registry& registry, float deltaTime) override;
    void render(Registry&, Renderer&) override {}

private:
    void createNormalShot(Registry& registry, float x, float y, float damage, bool piercing);
    void createChargedShot(Registry& registry, float x, float y, float damage, bool piercing);
    void createAngledShot(Registry& registry, float x, float y, float angleDegrees, float damage, bool piercing);

    float _windowWidth;
    float _chargeTime;
    bool _wasSpacePressed;
};