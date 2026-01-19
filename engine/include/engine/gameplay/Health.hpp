#pragma once

#include "engine/core/Component.hpp"

class Health : public Component {
public:
    Health();
    explicit Health(float maxHealth);
    ~Health() override = default;

    float current;
    float max;
};