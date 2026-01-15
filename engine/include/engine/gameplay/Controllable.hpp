#pragma once

#include "engine/core/Component.hpp"

class Controllable : public Component {
public:
    Controllable();
    explicit Controllable(float speed);
    ~Controllable() override = default;

    float speed;
    bool canShoot;
    float shootCooldown;
    float currentCooldown;
};
