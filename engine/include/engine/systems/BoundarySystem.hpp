#pragma once

#include "engine/core/System.hpp"

class BoundarySystem : public System {
public:
    BoundarySystem(float minX, float maxX, float minY, float maxY);

    void update(Registry& registry, float deltaTime) override;

private:
    float _minX;
    float _maxX;
    float _minY;
    float _maxY;
};