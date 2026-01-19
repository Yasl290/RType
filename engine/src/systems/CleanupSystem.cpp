#include "engine/systems/CleanupSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Projectile.hpp"
#include <vector>

CleanupSystem::CleanupSystem(float windowWidth)
    : _windowWidth(windowWidth)
{
}

void CleanupSystem::update(Registry& registry, float)
{
    std::vector<EntityID> toDestroy;

    registry.each<Transform, Projectile>([&](EntityID id, Transform& t, Projectile&) {
        if (t.x > _windowWidth + 100.f || t.x < -100.f) {
            toDestroy.push_back(id);
        }
    });

    for (EntityID id : toDestroy) {
        registry.markForDestruction(id);
    }
}