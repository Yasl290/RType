#include "engine/systems/EnemyShootingSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Enemy.hpp"
#include "engine/gameplay/EnemyShooter.hpp"
#include "engine/gameplay/Projectile.hpp"

EnemyShootingSystem::EnemyShootingSystem() = default;

void EnemyShootingSystem::update(Registry& registry, float deltaTime)
{
    registry.each<Transform, Enemy, EnemyShooter>(
        [&](EntityID /*enemyId*/, Transform& t, Enemy&, EnemyShooter& shooter) {
            if (shooter.shootInterval <= 0.f) {
                return;
            }

            if (shooter.currentCooldown > 0.f) {
                shooter.currentCooldown -= deltaTime;
            }

            if (shooter.currentCooldown > 0.f) {
                return;
            }

            EntityID bullet = registry.create();

            Sprite& sprite = registry.add<Sprite>(bullet);
            sprite.loadTexture("assets/sprites/enemy-shoot.png");

            float bulletX = t.x - 20.f;
            float bulletY = t.y + 40.f;

            Transform& bt = registry.add<Transform>(bullet, bulletX, bulletY);
            bt.scaleX = 1.0f;
            bt.scaleY = 1.0f;

            registry.add<Velocity>(bullet, -shooter.bulletSpeed, 0.f);

            Projectile& proj = registry.add<Projectile>(bullet);
            proj.type = ProjectileType::Normal;
            proj.damage = 10.f;
            proj.piercing = false;
            proj.isPlayerProjectile = false;

            shooter.currentCooldown = shooter.shootInterval;
        });
}