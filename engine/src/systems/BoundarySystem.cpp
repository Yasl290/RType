#include "engine/systems/BoundarySystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Controllable.hpp"

BoundarySystem::BoundarySystem(float minX, float maxX, float minY, float maxY)
    : _minX(minX), _maxX(maxX), _minY(minY), _maxY(maxY)
{
}

void BoundarySystem::update(Registry& registry, float)
{
    registry.each<Transform, Controllable>([this](EntityID, Transform& t, Controllable&) {
        if (t.x < _minX)
            t.x = _minX;
        if (t.x > _maxX)
            t.x = _maxX;
        if (t.y < _minY)
            t.y = _minY;
        if (t.y > _maxY)
            t.y = _maxY;
    });
}