#include "engine/systems/ShootingSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/Projectile.hpp"
#include "engine/gameplay/PlayerStats.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <iostream>
#include <cmath>

ShootingSystem::ShootingSystem(float windowWidth)
    : _windowWidth(windowWidth), _chargeTime(0.f), _wasSpacePressed(false)
{
}

void ShootingSystem::update(Registry& registry, float deltaTime)
{
    bool spacePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);

    registry.each<Transform, Controllable>([&](EntityID id, Transform& t, Controllable& c) {
        if (c.currentCooldown > 0.f) {
            c.currentCooldown -= deltaTime;
        }

        if (spacePressed) {
            _chargeTime += deltaTime;
        }

        if (!spacePressed && _wasSpacePressed) {
            if (c.canShoot && c.currentCooldown <= 0.f) {
                float shootX = t.x + 180.f;
                float shootY = t.y + 40.f;
                
                float baseDamage = 10.f;
                float baseCooldown = 0.25f;
                int shotCount = 1;
                bool piercing = false;
                
                if (registry.has<PlayerStats>(id)) {
                    PlayerStats& stats = registry.get<PlayerStats>(id);
                    baseDamage *= stats.getFinalDamageMultiplier();
                    baseCooldown *= stats.getFinalFireRateDivisor();
                    shotCount = stats.multishotCount;
                    piercing = stats.hasPiercingShots;
                }

                if (_chargeTime >= 2.0f) {
                    createChargedShot(registry, shootX, shootY, baseDamage * 5.0f, piercing);
                    c.currentCooldown = 1.5f * baseCooldown;
                } else if (_chargeTime > 0.f) {
                    if (shotCount == 1) {
                        createNormalShot(registry, shootX, shootY, baseDamage, piercing);
                    } else {
                        float spreadAngle = 15.0f;
                        float angleStep = (shotCount > 1) ? (2.0f * spreadAngle) / (shotCount - 1) : 0.0f;
                        float startAngle = -spreadAngle;
                        
                        for (int i = 0; i < shotCount; ++i) {
                            float angle = startAngle + (i * angleStep);
                            createAngledShot(registry, shootX, shootY, angle, baseDamage, piercing);
                        }
                    }
                    c.currentCooldown = baseCooldown;
                }

                _chargeTime = 0.f;
            }
        }
    });

    _wasSpacePressed = spacePressed;
}

void ShootingSystem::createNormalShot(Registry& registry, float x, float y, float damage, bool piercing)
{
    EntityID bullet = registry.create();

    Sprite& sprite = registry.add<Sprite>(bullet);
    sprite.loadTexture("assets/sprites/bullet_normal.png");

    Transform& transform = registry.add<Transform>(bullet, x, y - 21.f);
    transform.scaleX = 1.0f;
    transform.scaleY = 1.0f;

    registry.add<Velocity>(bullet, 700.f, 0.f);
    
    Projectile& proj = registry.add<Projectile>(bullet);
    proj.type = ProjectileType::Normal;
    proj.damage = damage;
    proj.piercing = piercing;
    proj.isPlayerProjectile = true;
}

void ShootingSystem::createChargedShot(Registry& registry, float x, float y, float damage, bool)
{
    EntityID bullet = registry.create();

    Sprite& sprite = registry.add<Sprite>(bullet);
    sprite.loadTexture("assets/sprites/bullet_charged.png");

    Transform& transform = registry.add<Transform>(bullet, x, y - 42.f);
    transform.scaleX = 1.0f;
    transform.scaleY = 1.0f;

    registry.add<Velocity>(bullet, 500.f, 0.f);
    
    Projectile& proj = registry.add<Projectile>(bullet);
    proj.type = ProjectileType::Charged;
    proj.damage = damage;
    proj.piercing = true;
    proj.isPlayerProjectile = true;
}

void ShootingSystem::createAngledShot(Registry& registry, float x, float y, float angleDegrees, float damage, bool piercing)
{
    EntityID bullet = registry.create();

    Sprite& sprite = registry.add<Sprite>(bullet);
    sprite.loadTexture("assets/sprites/bullet_normal.png");

    Transform& transform = registry.add<Transform>(bullet, x, y - 21.f);
    transform.scaleX = 1.0f;
    transform.scaleY = 1.0f;

    float angleRad = angleDegrees * 3.14159f / 180.0f;
    float baseSpeed = 700.f;
    float vx = baseSpeed * std::cos(angleRad);
    float vy = baseSpeed * std::sin(angleRad);

    registry.add<Velocity>(bullet, vx, vy);
    
    Projectile& proj = registry.add<Projectile>(bullet);
    proj.type = ProjectileType::Normal;
    proj.damage = damage;
    proj.piercing = piercing;
    proj.isPlayerProjectile = true;
}