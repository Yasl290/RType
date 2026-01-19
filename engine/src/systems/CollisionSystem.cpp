#include "engine/systems/CollisionSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Projectile.hpp"
#include "engine/gameplay/Enemy.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/PlayerInputState.hpp"
#include "engine/gameplay/Score.hpp"
#include "engine/gameplay/PlayerStats.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <vector>
#include <utility>
#include <iostream>

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
            if (registry.has<Enemy>(enemyId)) {
                Enemy& enemy = registry.get<Enemy>(enemyId);
                uint32_t basePoints = 0;
                
                switch (enemy.type) {
                    case EnemyType::Basic:
                        basePoints = 100;
                        break;
                    case EnemyType::Boss:
                        basePoints = 1000;
                        break;
                    default:
                        basePoints = 100;
                        break;
                }
                
                bool hasStats = false;
                registry.each<Controllable, Score, PlayerStats>([&](EntityID, Controllable&, Score& score, PlayerStats& stats) {
                    float multiplier = stats.getFinalScoreMultiplier();
                    uint32_t finalPoints = static_cast<uint32_t>(basePoints * multiplier);
                    score.addPoints(finalPoints);
                    score.incrementKills();
                    hasStats = true;
                });
                
                if (!hasStats) {
                    registry.each<Controllable, Score>([basePoints](EntityID, Controllable&, Score& score) {
                        score.addPoints(basePoints);
                        score.incrementKills();
                    });
                }
            }
            
            registry.markForDestruction(enemyId);
        }
    }

    for (const auto& [playerId, damage] : playerHits) {
        if (!registry.has<Health>(playerId)) {
            continue;
        }

        bool isInvincible = false;
        if (registry.has<PlayerInputState>(playerId)) {
            PlayerInputState& input = registry.get<PlayerInputState>(playerId);
            isInvincible = (input.inputFlags & (1 << 6)) != 0;
        }

        if (isInvincible) {
            continue;
        }

        float finalDamage = damage;
        if (registry.has<PlayerStats>(playerId)) {
            PlayerStats& stats = registry.get<PlayerStats>(playerId);
            finalDamage *= (1.0f - stats.damageReduction);
        }

        Health& health = registry.get<Health>(playerId);
        health.current -= finalDamage;

        if (health.current <= 0.f) {
            if (registry.has<Score>(playerId)) {
                Score& score = registry.get<Score>(playerId);
                std::cout << "[CollisionSystem] Player died with Score: " 
                          << score.getPoints() << ", Kills: " << score.getEnemiesKilled() << std::endl;
            }
            registry.markForDestruction(playerId);
        }
    }
}