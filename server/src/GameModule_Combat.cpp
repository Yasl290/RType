#include "GameModule.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/Projectile.hpp"
#include "engine/gameplay/Enemy.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Score.hpp"
#include "levels/ILevel.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
    inline bool rectsOverlap(float x1, float y1, float w1, float h1,
                             float x2, float y2, float w2, float h2)
    {
        return x1 < x2 + w2 && x1 + w1 > x2 &&
               y1 < y2 + h2 && y1 + h1 > y2;
    }
    
    float getEnemyWidth(EnemyType type) {
        switch (type) {
            case EnemyType::Boss: return 200.f;
            case EnemyType::Bomber: return 130.f;
            case EnemyType::FastShooter: return 100.f;
            case EnemyType::Basic:
            default: return 110.f;
        }
    }
    
    float getEnemyHeight(EnemyType type) {
        switch (type) {
            case EnemyType::Boss: return 200.f;
            case EnemyType::Bomber: return 85.f;
            case EnemyType::FastShooter: return 65.f;
            case EnemyType::Basic:
            default: return 75.f;
        }
    }
}

void GameModule::cleanupProjectiles()
{
    std::vector<EntityID> toRemove;
    _registry.each<Transform, Projectile>([&](EntityID id, Transform& transform, Projectile&) {
        if (transform.x > WORLD_WIDTH + PROJECTILE_MARGIN ||
            transform.x < -PROJECTILE_MARGIN) {
            toRemove.push_back(id);
        }
    });

    for (EntityID id : toRemove) {
        LocalGameEvent event;
        event.type = EventType::ENTITY_DESTROYED;
        event.entity_id = 1000 + id;
        event.related_id = 0;
        event.entity_type = 2;
        event.extra_data = 1;
        _eventQueue.push(event);

        _registry.destroy(id);
    }
}

void GameModule::spawnProjectile(float x, float y, bool charged, uint32_t owner_id)
{
    bool doubleDamage = false;
    auto stateIt = _playerStates.find(owner_id);
    if (stateIt != _playerStates.end()) {
        doubleDamage = stateIt->second.doubleDamage;
    }

    float baseDamage = charged ? 50.f : 10.f;
    float damage = doubleDamage ? (baseDamage * 2.0f) : baseDamage;

    EntityID proj = _registry.create();
    _registry.add<Transform>(proj, x, y);
    _registry.add<Velocity>(proj, charged ? 500.f : 700.f, 0.f);
    Projectile& p = _registry.add<Projectile>(proj, charged ? ProjectileType::Charged : ProjectileType::Normal,
                                              damage, charged, owner_id);
    p.isPlayerProjectile = true;

    LocalGameEvent event;
    event.type = EventType::ENTITY_FIRED;
    event.entity_id = 1000 + proj;
    event.related_id = owner_id;
    event.pos_x = x;
    event.pos_y = y;
    event.entity_type = charged ? 3 : 2;
    event.extra_data = charged ? 1 : 0;
    _eventQueue.push(event);
}

void GameModule::spawnEnemyProjectile(float x, float y, float vx, float vy, float damage)
{
    EntityID proj = _registry.create();
    _registry.add<Transform>(proj, x, y);
    _registry.add<Velocity>(proj, vx, vy);
    Projectile& p = _registry.add<Projectile>(proj, ProjectileType::Normal, damage, false);
    p.isPlayerProjectile = false;

    LocalGameEvent event;
    event.type = EventType::ENTITY_FIRED;
    event.entity_id = 1000 + proj;
    event.related_id = 0;
    event.pos_x = x;
    event.pos_y = y;
    event.entity_type = 4;
    event.extra_data = 0;
    _eventQueue.push(event);
}

void GameModule::updateEnemies(float dt, ILevel* level)
{
    if (level->shouldSpawnEnemy(dt) && !_bossSpawned) {
        int enemyCount = 0;
        _registry.each<Enemy>([&](EntityID, Enemy& e) {
            if (e.type != EnemyType::Boss) {
                ++enemyCount;
            }
        });

        if (enemyCount < 8) {
            auto config = level->getEnemySpawn(_rng);
            spawnEnemy(config);
        }
    }

    {
        std::vector<EntityID> toRemove;
        _registry.each<Transform, Enemy>([&](EntityID id, Transform& t, Enemy& e) {
            if (e.type == EnemyType::Boss)
                return;
            constexpr float ENEMY_OFFSCREEN_MARGIN = 120.f;
            if (t.x < -ENEMY_OFFSCREEN_MARGIN) {
                toRemove.push_back(id);
            }
        });

        for (EntityID id : toRemove) {
            LocalGameEvent event;
            event.type = EventType::ENTITY_DESTROYED;
            event.entity_id = 1000 + id;
            event.related_id = 0;
            event.entity_type = 1;
            event.extra_data = 0;
            _eventQueue.push(event);

            _registry.destroy(id);
            _enemyShootTimers.erase(id);
        }
    }

    _registry.each<Transform, Velocity, Enemy>([&](EntityID, Transform& t, Velocity& v, Enemy& e) {
        if (e.type != EnemyType::Boss)
            return;
        if (t.x <= BOSS_TARGET_X) {
            t.x = BOSS_TARGET_X;
            v.x = 0.f;
        }
    });

    _registry.each<Transform, Enemy>([&](EntityID id, Transform& t, Enemy& e) {
        float& timer = _enemyShootTimers[id];
        timer -= dt;
        
        if (timer <= 0.f) {
            if (e.type == EnemyType::Boss) {
                level->updateBossBehavior(dt, id, _registry, _rng,
                    [this](float x, float y, float vx, float vy) {
                        spawnEnemyProjectile(x, y, vx, vy, 10.f);
                    });
                
                if (_registry.has<Health>(id)) {
                    const Health& h = _registry.get<Health>(id);
                    float ratio = (h.max > 0.f) ? (h.current / h.max) : 0.f;
                    
                    if (ratio >= 0.66f) {
                        timer = 1.5f; 
                    } else if (ratio >= 0.33f) {
                        timer = 2.0f;
                    } else {
                        timer = 0.8f;
                    }
                } else {
                    timer = 1.5f;
                }
            } else if (e.type == EnemyType::FastShooter) {
                float shootX = t.x - 20.f;
                float shootY = t.y + 33.f;
                spawnEnemyProjectile(shootX, shootY, -300.f, 0.f, 8.f);
                spawnEnemyProjectile(shootX, shootY - 10.f, -300.f, 0.f, 8.f);
                timer = level->getEnemyShootInterval(e.type);
            } else if (e.type == EnemyType::Bomber) {
                float shootX = t.x - 20.f;
                float shootY = t.y + 42.f;
                spawnEnemyProjectile(shootX, shootY, -200.f, 0.f, 25.f);
                timer = level->getEnemyShootInterval(e.type);
            } else {
                float shootX = t.x - 20.f;
                float shootY = t.y + 37.f;
                spawnEnemyProjectile(shootX, shootY, -250.f, 0.f, 10.f);
                timer = level->getEnemyShootInterval(e.type);
            }
        }
    });
}

void GameModule::spawnEnemy(const EnemySpawnConfig& config)
{
    EntityID enemy = _registry.create();
    _registry.add<Transform>(enemy, config.spawnX, config.spawnY);
    _registry.add<Velocity>(enemy, config.velocityX, config.velocityY);
    _registry.add<Enemy>(enemy, config.type);
    _registry.add<Health>(enemy, config.health);

    int entityType = 1;
    if (config.type == EnemyType::FastShooter) {
        entityType = 6;
    } else if (config.type == EnemyType::Bomber) {
        entityType = 7;
    }

    LocalGameEvent event;
    event.type = EventType::ENTITY_SPAWNED;
    event.entity_id = 1000 + enemy;
    event.entity_type = entityType;
    event.pos_x = config.spawnX;
    event.pos_y = config.spawnY;
    event.related_id = 0;
    event.extra_data = 0;
    _eventQueue.push(event);
}

void GameModule::spawnBoss(const BossSpawnConfig& config)
{
    EntityID boss = _registry.create();
    _registry.add<Transform>(boss, config.spawnX, config.spawnY);
    _registry.add<Velocity>(boss, config.velocityX, 0.f);
    _registry.add<Enemy>(boss, EnemyType::Boss);
    _registry.add<Health>(boss, config.health);

    _bossEntity = boss;

    LocalGameEvent event;
    event.type = EventType::ENTITY_SPAWNED;
    event.entity_id = 1000 + boss;
    event.entity_type = 5;
    event.pos_x = config.spawnX;
    event.pos_y = config.spawnY;
    event.related_id = 0;
    event.extra_data = 0;
    _eventQueue.push(event);
}

void GameModule::handleEnemyCollisions()
{
    constexpr float PLAYER_W = 100.f;
    constexpr float PLAYER_H = 60.f;
    constexpr float BULLET_W = 20.f;
    constexpr float BULLET_H = 20.f;

    std::vector<std::pair<EntityID, uint32_t>> enemiesToKill;
    std::vector<EntityID> projectilesToKill;
    std::vector<std::pair<EntityID, uint32_t>> playersToKill;

    _registry.each<Transform, Enemy, Health>([&](EntityID enemyId, Transform& eT, Enemy& enemy, Health& eH) {
        float enemyW = getEnemyWidth(enemy.type);
        float enemyH = getEnemyHeight(enemy.type);
        
        _registry.each<Transform, Projectile>([&](EntityID projId, Transform& pT, Projectile& proj) {
            if (!proj.isPlayerProjectile) {
                return;
            }

            if (rectsOverlap(eT.x, eT.y, enemyW, enemyH,
                             pT.x, pT.y, BULLET_W, BULLET_H)) {
                eH.current -= proj.damage;
                if (!proj.piercing) {
                    projectilesToKill.push_back(projId);
                }
                if (eH.current <= 0.f) {
                    enemiesToKill.push_back({enemyId, proj.ownerId});
                }
            }
        });
    });

    _registry.each<Transform, Projectile>([&](EntityID projId, Transform& pT, Projectile& proj) {
        if (proj.isPlayerProjectile) {
            return;
        }

        _registry.each<Transform, Controllable, Health>([&](EntityID playerId, Transform& t, Controllable&, Health& h) {
            if (rectsOverlap(t.x, t.y, PLAYER_W, PLAYER_H,
                             pT.x, pT.y, BULLET_W, BULLET_H)) {
                uint32_t client_id = 0;
                for (const auto& [cid, eid] : _playerEntities) {
                    if (eid == playerId) {
                        client_id = cid;
                        break;
                    }
                }

                bool invincible = false;
                if (client_id != 0) {
                    auto stateIt = _playerStates.find(client_id);
                    if (stateIt != _playerStates.end()) {
                        invincible = stateIt->second.invincibility;
                    }
                }

                if (!invincible) {
                    h.current -= proj.damage;
                }

                if (!proj.piercing) {
                    projectilesToKill.push_back(projId);
                }

                if (!invincible && h.current <= 0.f) {
                    playersToKill.push_back({playerId, 0});
                }
            }
        });
    });

    _registry.each<Transform, Enemy, Health>([&](EntityID enemyId, Transform& eT, Enemy& enemy, Health&) {
        float enemyW = getEnemyWidth(enemy.type);
        float enemyH = getEnemyHeight(enemy.type);
        
        _registry.each<Transform, Controllable, Health>([&](EntityID playerId, Transform& t, Controllable&, Health& h) {
            if (rectsOverlap(eT.x, eT.y, enemyW, enemyH,
                             t.x, t.y, PLAYER_W, PLAYER_H)) {
                uint32_t client_id = 0;
                for (const auto& [cid, eid] : _playerEntities) {
                    if (eid == playerId) {
                        client_id = cid;
                        break;
                    }
                }

                bool invincible = false;
                if (client_id != 0) {
                    auto stateIt = _playerStates.find(client_id);
                    if (stateIt != _playerStates.end()) {
                        invincible = stateIt->second.invincibility;
                    }
                }

                if (!invincible) {
                    h.current -= ENEMY_CONTACT_DAMAGE;
                }

                if (enemy.type != EnemyType::Boss) {
                    enemiesToKill.push_back({enemyId, 0});
                }

                if (!invincible && h.current <= 0.f) {
                    playersToKill.push_back({playerId, 1000 + enemyId});
                }
            }
        });
    });

    for (const auto& [enemyId, killer_id] : enemiesToKill) {
        LocalGameEvent event;
        event.type = EventType::ENTITY_DESTROYED;
        event.entity_id = 1000 + enemyId;
        event.related_id = killer_id;
        event.entity_type = 1;
        event.extra_data = 0;
        _eventQueue.push(event);

        _registry.destroy(enemyId);
        _enemyShootTimers.erase(enemyId);

        if (killer_id != 0) {
            auto it = _playerEntities.find(killer_id);
            if (it != _playerEntities.end() && _registry.has<Score>(it->second)) {
                Score& score = _registry.get<Score>(it->second);
                score.addPoints(ENEMY_KILL_POINTS);
                score.incrementKills();

                _persistentScores[killer_id] = score.getPoints();
                _persistentKills[killer_id] = score.getEnemiesKilled();
            }
        }
    }

    for (EntityID id : projectilesToKill) {
        LocalGameEvent event;
        event.type = EventType::ENTITY_DESTROYED;
        event.entity_id = 1000 + id;
        event.related_id = 0;
        event.entity_type = 2;
        event.extra_data = 0;
        _eventQueue.push(event);

        _registry.destroy(id);
    }

    for (const auto& [playerId, killerId] : playersToKill) {
        uint32_t client_id = 0;
        for (const auto& [cid, entity_id] : _playerEntities) {
            if (entity_id == playerId) {
                client_id = cid;
                savePlayerScore(client_id);
                break;
            }
        }

        if (client_id != 0) {
            LocalGameEvent event;
            event.type = EventType::PLAYER_DIED;
            event.entity_id = client_id;
            event.related_id = killerId;
            event.entity_type = 0;
            event.extra_data = killerId >= 1000 ? 1 : 0;
            _eventQueue.push(event);
        }

        _registry.destroy(playerId);
    }
}

void GameModule::scheduleNextEnemySpawn()
{
    _enemySpawnInterval = _enemySpawnIntervalDist(_rng);
}