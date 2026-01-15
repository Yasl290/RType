#include "engine/systems/CollisionSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Projectile.hpp"
#include "engine/gameplay/Enemy.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Controllable.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <vector>
#include <utility>

CollisionSystem::CollisionSystem() = default;

void CollisionSystem::update(Registry& registry, float)
{

    std::vector<std::pair<EntityID, float>> enemyHits; 
    std::vector<std::pair<EntityID, float>> playerHits;
    std::vector<EntityID> projectilesToDestroy;

    constexpr float CONTACT_DAMAGE = 20.f;

    registry.each<Transform, Sprite, Enemy, Health>(
        [&](EntityID enemyId, Transform& enemyTransform, Sprite& enemySprite, Enemy&, Health&) {
            sf::FloatRect enemyBounds;

            enemySprite.setPosition(enemyTransform.x, enemyTransform.y);
            enemySprite.setScale(enemyTransform.scaleX, enemyTransform.scaleY);
            enemyBounds = enemySprite.getSprite().getGlobalBounds();

            registry.each<Transform, Sprite, Projectile>(
                [&](EntityID projectileId, Transform& projTransform, Sprite& projSprite, Projectile& projectile) {
                    sf::FloatRect projBounds;

                    if (!projectile.isPlayerProjectile) {
                        return;
                    }

                    projSprite.setPosition(projTransform.x, projTransform.y);
                    projSprite.setScale(projTransform.scaleX, projTransform.scaleY);
                    projBounds = projSprite.getSprite().getGlobalBounds();

                    if (projBounds.intersects(enemyBounds)) {
                        enemyHits.emplace_back(enemyId, projectile.damage);

                        if (!projectile.piercing) {
                            projectilesToDestroy.push_back(projectileId);
                        }
                    }
                });

            registry.each<Transform, Sprite, Controllable, Health>(
                [&](EntityID playerId, Transform& playerTransform, Sprite& playerSprite, Controllable&, Health&) {
                    playerSprite.setPosition(playerTransform.x, playerTransform.y);
                    playerSprite.setScale(playerTransform.scaleX, playerTransform.scaleY);
                    sf::FloatRect playerBounds = playerSprite.getSprite().getGlobalBounds();

                    if (playerBounds.intersects(enemyBounds)) {
                        playerHits.emplace_back(playerId, CONTACT_DAMAGE);
                        registry.markForDestruction(enemyId);
                    }
                });
        });

    registry.each<Transform, Sprite, Projectile>(
        [&](EntityID projectileId, Transform& projTransform, Sprite& projSprite, Projectile& projectile) {
            sf::FloatRect projBounds;

            if (projectile.isPlayerProjectile) {
                return;
            }

            projSprite.setPosition(projTransform.x, projTransform.y);
            projSprite.setScale(projTransform.scaleX, projTransform.scaleY);
            projBounds = projSprite.getSprite().getGlobalBounds();

            registry.each<Transform, Sprite, Controllable, Health>(
                [&](EntityID playerId, Transform& playerTransform, Sprite& playerSprite, Controllable&, Health&) {
                    sf::FloatRect playerBounds;

                    playerSprite.setPosition(playerTransform.x, playerTransform.y);
                    playerSprite.setScale(playerTransform.scaleX, playerTransform.scaleY);
                    playerBounds = playerSprite.getSprite().getGlobalBounds();

                    if (projBounds.intersects(playerBounds)) {
                        playerHits.emplace_back(playerId, projectile.damage);
                        if (!projectile.piercing) {
                            projectilesToDestroy.push_back(projectileId);
                        }
                    }
                });
        });

    for (EntityID id : projectilesToDestroy) {
        registry.markForDestruction(id);
    }

    for (const auto& [enemyId, damage] : enemyHits) {
        if (!registry.has<Health>(enemyId)) {
            continue;
        }

        Health& health = registry.get<Health>(enemyId);
        health.current -= damage;

        if (health.current <= 0.f) {
            registry.markForDestruction(enemyId);
        }
    }

    for (const auto& [playerId, damage] : playerHits) {
        if (!registry.has<Health>(playerId)) {
            continue;
        }

        Health& health = registry.get<Health>(playerId);
        health.current -= damage;

        if (health.current <= 0.f) {
            registry.markForDestruction(playerId);
        }
    }
}
