#include "engine/systems/ServerShootingSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/Projectile.hpp"
#include "engine/gameplay/PlayerInputState.hpp"
#include <iostream>

ServerShootingSystem::ServerShootingSystem(float windowWidth)
    : _windowWidth(windowWidth), _chargeTime(0.f), _wasShooting(false)
{
}

void ServerShootingSystem::update(Registry& registry, float deltaTime)
{
    bool anyShooting = false;

    registry.each<Transform, Controllable, PlayerInputState>(
        [&](EntityID, Transform& t, Controllable& c, PlayerInputState& input) {
            constexpr uint8_t SHOOT = 1u << 4;
            bool shooting = (input.inputFlags & SHOOT) != 0;
            bool doubleFireRate = (input.inputFlags & (1 << 7)) != 0;
            bool doubleDamage = (input.inputFlags & (1 << 5)) != 0;

            if (c.currentCooldown > 0.f) {
                float cooldownMultiplier = doubleFireRate ? 2.0f : 1.0f;
                c.currentCooldown -= deltaTime * cooldownMultiplier;
            }

            if (shooting) {
                anyShooting = true;
                _chargeTime += deltaTime;
            }

            if (!shooting && _wasShooting) {
                if (c.canShoot && c.currentCooldown <= 0.f) {
                    float shootX = t.x + 180.f;
                    float shootY = t.y + 40.f;

                    if (_chargeTime >= 2.0f) {
                        float damage = doubleDamage ? 100.f : 50.f;
                        createChargedShot(registry, shootX, shootY, damage);
                        float cooldown = doubleFireRate ? 0.75f : 1.5f;
                        c.currentCooldown = cooldown;
                        std::cout << "[Server] CHARGED SHOT!" << std::endl;
                    } else if (_chargeTime > 0.f) {
                        float damage = doubleDamage ? 20.f : 10.f;
                        createNormalShot(registry, shootX, shootY, damage);
                        float cooldown = doubleFireRate ? 0.125f : 0.25f;
                        c.currentCooldown = cooldown;
                        std::cout << "[Server] Normal shot" << std::endl;
                    }

                    _chargeTime = 0.f;
                }
            }
        });

    _wasShooting = anyShooting;
}

void ServerShootingSystem::createNormalShot(Registry& registry, float x, float y, float damage)
{
    EntityID bullet = registry.create();

    Sprite& sprite = registry.add<Sprite>(bullet);
    sprite.loadTexture("assets/sprites/bullet_normal.png");

    Transform& transform = registry.add<Transform>(bullet, x, y - 21.f);
    transform.scaleX = 1.0f;
    transform.scaleY = 1.0f;

    registry.add<Velocity>(bullet, 700.f, 0.f);
    registry.add<Projectile>(bullet, ProjectileType::Normal, damage, false);
}

void ServerShootingSystem::createChargedShot(Registry& registry, float x, float y, float damage)
{
    EntityID bullet = registry.create();

    Sprite& sprite = registry.add<Sprite>(bullet);
    sprite.loadTexture("assets/sprites/bullet_charged.png");

    Transform& transform = registry.add<Transform>(bullet, x, y - 42.f);
    transform.scaleX = 1.0f;
    transform.scaleY = 1.0f;

    registry.add<Velocity>(bullet, 500.f, 0.f);
    registry.add<Projectile>(bullet, ProjectileType::Charged, damage, true);
}