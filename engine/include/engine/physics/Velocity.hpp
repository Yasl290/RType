#pragma once

#include "engine/core/Component.hpp"

class Velocity : public Component {
public:
    Velocity();
    Velocity(float vx, float vy);
    ~Velocity() override = default;

    float x;
    float y;
};