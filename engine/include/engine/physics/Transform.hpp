#pragma once

#include "engine/core/Component.hpp"

class Transform : public Component {
public:
    Transform();
    Transform(float x, float y);
    ~Transform() override = default;

    float x;
    float y;
    float rotation;
    float scaleX;
    float scaleY;
};