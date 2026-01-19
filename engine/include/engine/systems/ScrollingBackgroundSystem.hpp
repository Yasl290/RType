#pragma once

#include "engine/core/System.hpp"

class ScrollingBackgroundSystem : public System {
public:
    explicit ScrollingBackgroundSystem(float windowWidth);

    void update(Registry& registry, float deltaTime) override;

private:
    float _windowWidth;
};