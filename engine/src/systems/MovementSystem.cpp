#include "engine/systems/MovementSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"

MovementSystem::MovementSystem() = default;

void MovementSystem::update(Registry& registry, float deltaTime)
{
    registry.each<Transform, Velocity>([deltaTime](EntityID, Transform& t, Velocity& v) {
        t.x += v.x * deltaTime;
        t.y += v.y * deltaTime;
    });
}