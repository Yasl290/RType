#include "GameModule.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/Projectile.hpp"
#include "engine/gameplay/Enemy.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Score.hpp"
#include <iostream>
#include <algorithm>
#include <vector>

namespace {
    inline bool rectsOverlap(float x1, float y1, float w1, float h1,
                             float x2, float y2, float w2, float h2)
    {
        return x1 < x2 + w2 && x1 + w1 > x2 &&
               y1 < y2 + h2 && y1 + h1 > y2;
    }
}

GameModule::GameModule()
    : _enemySpawnTimer(0.f),
      _enemySpawnInterval(1.f),
      _rng(std::random_device{}()),
      _enemySpawnIntervalDist(0.5f, 3.0f),
      _enemySpawnYDist(50.f, WORLD_HEIGHT - 100.f)
{
}

void GameModule::init()
{
    _playerStates.clear();
    _playerEntities.clear();
    _movementSystem = std::make_unique<MovementSystem>();

    _enemySpawnTimer = 0.f;
    scheduleNextEnemySpawn();
    _enemyShootTimers.clear();
    
    _persistentScores.clear();
    _persistentKills.clear();
    
    while (!_eventQueue.empty()) {
        _eventQueue.pop();
    }
    
    _registry.each<Transform>([&](EntityID id, Transform&) {
        _registry.markForDestruction(id);
    });
    _registry.cleanup();
    
    std::cout << "[GameModule] Fully reset - all entities cleared" << std::endl;
}

void GameModule::update(float dt)
{
    _movementSystem->update(_registry, dt);

    _registry.each<Controllable>([dt](EntityID, Controllable& c) {
        if (!c.canShoot) {
            c.currentCooldown -= dt;
            if (c.currentCooldown <= 0.f) {
                c.canShoot = true;
                c.currentCooldown = 0.f;
            }
        }
    });

    updateChargeStates(dt);
    updateEnemies(dt);
    handleEnemyCollisions();
    cleanupProjectiles();
    _registry.cleanup();
}

uint32_t GameModule::spawnPlayer(uint32_t client_id, float x, float y, uint8_t player_slot)
{
    if (_playerEntities.size() >= MAX_PLAYERS) {
        std::cerr << "[GameModule] Max players reached (" << MAX_PLAYERS << "/" << MAX_PLAYERS << ")" << std::endl;
        return 0;
    }

    if (_playerEntities.find(client_id) != _playerEntities.end()) {
        std::cerr << "[GameModule] Player " << client_id << " already exists" << std::endl;
        return _playerEntities[client_id];
    }

    EntityID player = _registry.create();    
    _registry.add<Transform>(player, x, y);
    _registry.add<Velocity>(player, 0.f, 0.f);
    _registry.add<Controllable>(player, 250.f);
    _registry.add<Health>(player, 100.f);
    
    uint32_t initialScore = 0;
    uint32_t initialKills = 0;
    auto scoreIt = _persistentScores.find(client_id);
    if (scoreIt != _persistentScores.end()) {
        initialScore = scoreIt->second;
    }
    auto killsIt = _persistentKills.find(client_id);
    if (killsIt != _persistentKills.end()) {
        initialKills = killsIt->second;
    }
    
    _registry.add<Score>(player, initialScore, initialKills);
    
    _playerEntities[client_id] = player;
    _playerStates[client_id] = {};
    _playerStates[client_id].slot = player_slot;

    GameEvent event;
    event.type = EventType::ENTITY_SPAWNED;
    event.entity_id = client_id;
    event.entity_type = 0;
    event.pos_x = x;
    event.pos_y = y;
    event.related_id = 0;
    event.extra_data = 0;
    _eventQueue.push(event);

    return player;
}

uint32_t GameModule::getPlayerScore(uint32_t client_id)
{
    auto persistentIt = _persistentScores.find(client_id);
    if (persistentIt != _persistentScores.end()) {
        return persistentIt->second;
    }
    
    auto it = _playerEntities.find(client_id);
    if (it == _playerEntities.end()) {
        return 0;
    }
    
    EntityID player = it->second;
    if (!_registry.has<Score>(player)) {
        return 0;
    }
    
    const Score& score = _registry.get<Score>(player);
    return score.getPoints();
}

uint32_t GameModule::getPlayerKills(uint32_t client_id)
{
    auto persistentIt = _persistentKills.find(client_id);
    if (persistentIt != _persistentKills.end()) {
        return persistentIt->second;
    }
    
    auto it = _playerEntities.find(client_id);
    if (it == _playerEntities.end()) {
        return 0;
    }
    
    EntityID player = it->second;
    if (!_registry.has<Score>(player)) {
        return 0;
    }
    
    const Score& score = _registry.get<Score>(player);
    return score.getEnemiesKilled();
}

void GameModule::savePlayerScore(uint32_t client_id)
{
    auto it = _playerEntities.find(client_id);
    if (it == _playerEntities.end()) {
        return;
    }
    
    EntityID player = it->second;
    if (_registry.has<Score>(player)) {
        const Score& score = _registry.get<Score>(player);
        _persistentScores[client_id] = score.getPoints();
        _persistentKills[client_id] = score.getEnemiesKilled();
        
        std::cout << "[GameModule] Saved score for player " << client_id 
                  << ": " << score.getPoints() << " pts, " 
                  << score.getEnemiesKilled() << " kills" << std::endl;
    }
}

void GameModule::removePlayer(uint32_t client_id)
{
    auto it = _playerEntities.find(client_id);
    if (it != _playerEntities.end()) {
        savePlayerScore(client_id);
        _registry.destroy(it->second);
        _playerEntities.erase(it);
        _playerStates.erase(client_id);
    }
}

void GameModule::processInput(uint32_t client_id, uint8_t flags)
{
    auto it = _playerEntities.find(client_id);
    if (it == _playerEntities.end()) {
        return;
    }
    EntityID player = it->second;
    if (!_registry.has<Velocity>(player) || 
        !_registry.has<Controllable>(player) || 
        !_registry.has<Transform>(player)) {
        return;
    }

    Velocity& velocity = _registry.get<Velocity>(player);
    Controllable& controllable = _registry.get<Controllable>(player);
    PlayerState& state = _playerStates[client_id];
    velocity.x = 0.f;
    velocity.y = 0.f;
    
    float speed = controllable.speed;
    if (flags & 0x01) {
        velocity.y = -speed;
    }
    if (flags & 0x02) {
        velocity.y = +speed;
    }
    if (flags & 0x04) {
        velocity.x = -speed;
    }
    if (flags & 0x08) {
        velocity.x = +speed;
    }

    bool shootPressed = (flags & 0x10) != 0;
    if (shootPressed) {
        if (!state.isCharging && controllable.canShoot) {
            state.isCharging = true;
            state.chargeTime = 0.f;
        }
    } else if (state.isCharging) {
        state.isCharging = false;
        handleShootRelease(client_id, state);
    }
}

std::vector<EntitySnapshot> GameModule::getWorldSnapshot()
{
    std::vector<EntitySnapshot> snapshots;

    std::unordered_map<EntityID, uint32_t> entityToClient;
    for (const auto& [client_id, entity_id] : _playerEntities) {
        entityToClient[entity_id] = client_id;
    }

    _registry.each<Transform>([&](EntityID id, const Transform& transform) {
        EntitySnapshot snap;
        auto it = entityToClient.find(id);
        if (it != entityToClient.end()) {
            snap.entity_id = it->second;
            auto stateIt = _playerStates.find(it->second);
            snap.player_slot = (stateIt != _playerStates.end()) ? stateIt->second.slot : 0;
        } else {
            snap.entity_id = 1000 + id;
            snap.player_slot = 255;
        }
        
        snap.pos_x = transform.x;
        snap.pos_y = transform.y;
        snap.vel_x = 0.f;
        snap.vel_y = 0.f;
        
        if (_registry.has<Velocity>(id)) {
            const Velocity& velocity = _registry.get<Velocity>(id);
            snap.vel_x = velocity.x;
            snap.vel_y = velocity.y;
        }

        if (_registry.has<Projectile>(id)) {
            const Projectile& proj = _registry.get<Projectile>(id);
            if (!proj.isPlayerProjectile) {
                snap.entity_type = 4;
            } else {
                snap.entity_type = (proj.type == ProjectileType::Charged) ? 3 : 2;
            }
        } else if (_registry.has<Enemy>(id)) {
            snap.entity_type = 1;
        } else {
            snap.entity_type = 0;
        }

        if (_registry.has<Health>(id)) {
            const Health& h = _registry.get<Health>(id);
            snap.hp_current = h.current;
            snap.hp_max = h.max;
        } else {
            snap.hp_current = 0.f;
            snap.hp_max = 0.f;
        }
        
        snapshots.push_back(snap);
    });
    
    return snapshots;
}

std::vector<GameEvent> GameModule::pollEvents()
{
    std::vector<GameEvent> events;
    while (!_eventQueue.empty()) {
        events.push_back(_eventQueue.front());
        _eventQueue.pop();
    }
    return events;
}

void GameModule::handleShootRelease(uint32_t client_id, PlayerState& state)
{
    auto entityIt = _playerEntities.find(client_id);
    if (entityIt == _playerEntities.end()) {
        return;
    }
    
    EntityID player = entityIt->second;
    if (!_registry.has<Controllable>(player) || !_registry.has<Transform>(player)) {
        return;
    }
    
    Controllable& controllable = _registry.get<Controllable>(player);
    Transform& transform = _registry.get<Transform>(player);
    
    if (!controllable.canShoot) {
        state.chargeTime = 0.f;
        return;
    }
    
    bool charged = state.chargeTime >= CHARGED_SHOT_THRESHOLD;
    
    float shootX = transform.x + 60.f;
    float shootY = transform.y + 20.f;
    spawnProjectile(shootX, shootY, charged, client_id);
    
    controllable.canShoot = false;
    controllable.currentCooldown = charged ? CHARGED_SHOT_COOLDOWN : NORMAL_SHOT_COOLDOWN;
    state.chargeTime = 0.f;
}

void GameModule::updateChargeStates(float dt)
{
    for (auto& [client_id, state] : _playerStates) {
        if (state.isCharging) {
            state.chargeTime = std::min(state.chargeTime + dt, MAX_CHARGE_TIME);
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
        GameEvent event;
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
    EntityID proj = _registry.create();
    _registry.add<Transform>(proj, x, y);
    _registry.add<Velocity>(proj, charged ? 500.f : 700.f, 0.f);
    Projectile& p = _registry.add<Projectile>(proj, charged ? ProjectileType::Charged : ProjectileType::Normal,
                                              charged ? 50.f : 10.f, charged, owner_id);
    p.isPlayerProjectile = true;
    
    GameEvent event;
    event.type = EventType::ENTITY_FIRED;
    event.entity_id = 1000 + proj;
    event.related_id = owner_id;
    event.pos_x = x;
    event.pos_y = y;
    event.entity_type = charged ? 3 : 2;
    event.extra_data = charged ? 1 : 0;
    _eventQueue.push(event);
}

void GameModule::spawnEnemyProjectile(float x, float y)
{
    EntityID proj = _registry.create();
    _registry.add<Transform>(proj, x, y);
    _registry.add<Velocity>(proj, -250.f, 0.f);
    Projectile& p = _registry.add<Projectile>(proj, ProjectileType::Normal, 10.f, false);
    p.isPlayerProjectile = false;
    
    GameEvent event;
    event.type = EventType::ENTITY_FIRED;
    event.entity_id = 1000 + proj;
    event.related_id = 0;
    event.pos_x = x;
    event.pos_y = y;
    event.entity_type = 4;
    event.extra_data = 0;
    _eventQueue.push(event);
}

void GameModule::updateEnemies(float dt)
{
    _enemySpawnTimer += dt;
    if (_enemySpawnTimer >= _enemySpawnInterval) {
        spawnEnemy();
        _enemySpawnTimer = 0.f;
        scheduleNextEnemySpawn();
    }

    _registry.each<Transform, Enemy>([&](EntityID id, Transform& t, Enemy&) {
        float& timer = _enemyShootTimers[id];

        timer -= dt;
        if (timer <= 0.f) {
            float shootX = t.x - 20.f;
            float shootY = t.y + 20.f;
            spawnEnemyProjectile(shootX, shootY);
            timer = ENEMY_SHOOT_INTERVAL;
        }
    });
}

void GameModule::spawnEnemy()
{
    float spawnX = WORLD_WIDTH + 150.f;
    float spawnY = _enemySpawnYDist(_rng);

    EntityID enemy = _registry.create();
    _registry.add<Transform>(enemy, spawnX, spawnY);
    _registry.add<Velocity>(enemy, -ENEMY_SPEED, 0.f);
    _registry.add<Enemy>(enemy, EnemyType::Basic);
    _registry.add<Health>(enemy, 50.f);
    
    GameEvent event;
    event.type = EventType::ENTITY_SPAWNED;
    event.entity_id = 1000 + enemy;
    event.entity_type = 1;
    event.pos_x = spawnX;
    event.pos_y = spawnY;
    event.related_id = 0;
    event.extra_data = 0;
    _eventQueue.push(event);
}

void GameModule::handleEnemyCollisions()
{
    constexpr float ENEMY_W = 100.f;
    constexpr float ENEMY_H = 60.f;
    constexpr float PLAYER_W = 100.f;
    constexpr float PLAYER_H = 60.f;
    constexpr float BULLET_W = 20.f;
    constexpr float BULLET_H = 20.f;

    std::vector<std::pair<EntityID, uint32_t>> enemiesToKill;
    std::vector<EntityID> projectilesToKill;
    std::vector<std::pair<EntityID, uint32_t>> playersToKill;

    _registry.each<Transform, Enemy, Health>([&](EntityID enemyId, Transform& eT, Enemy&, Health& eH) {
        _registry.each<Transform, Projectile>([&](EntityID projId, Transform& pT, Projectile& proj) {
            if (!proj.isPlayerProjectile) {
                return;
            }

            if (rectsOverlap(eT.x, eT.y, ENEMY_W, ENEMY_H,
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
                h.current -= proj.damage;
                if (!proj.piercing) {
                    projectilesToKill.push_back(projId);
                }
                if (h.current <= 0.f) {
                    playersToKill.push_back({playerId, 0});
                }
            }
        });
    });

    _registry.each<Transform, Enemy, Health>([&](EntityID enemyId, Transform& eT, Enemy&, Health&) {
        _registry.each<Transform, Controllable, Health>([&](EntityID playerId, Transform& t, Controllable&, Health& h) {
            if (rectsOverlap(eT.x, eT.y, ENEMY_W, ENEMY_H,
                             t.x, t.y, PLAYER_W, PLAYER_H)) {
                h.current -= ENEMY_CONTACT_DAMAGE;
                enemiesToKill.push_back({enemyId, 0});
                if (h.current <= 0.f) {
                    playersToKill.push_back({playerId, 1000 + enemyId});
                }
            }
        });
    });

    for (const auto& [enemyId, killer_id] : enemiesToKill) {
        GameEvent event;
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
        GameEvent event;
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
            GameEvent event;
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

bool GameModule::areAllPlayersDead()
{
    if (_playerEntities.empty()) {
        return false;
    }
    
    for (const auto& [client_id, entity_id] : _playerEntities) {
        if (_registry.has<Health>(entity_id)) {
            const Health& health = _registry.get<Health>(entity_id);
            if (health.current > 0.f) {
                return false;
            }
        }
    }
    
    for (const auto& [client_id, entity_id] : _playerEntities) {
        savePlayerScore(client_id);
    }
    
    return true;
}

std::vector<RType::Protocol::PlayerFinalScore> GameModule::getFinalScores()
{
    std::vector<RType::Protocol::PlayerFinalScore> scores;
    
    for (const auto& [client_id, entity_id] : _playerEntities) {
        RType::Protocol::PlayerFinalScore score;
        score.client_id = client_id;
        
        std::string name = "Player " + std::to_string(client_id);
        std::strncpy(score.player_name, name.c_str(), sizeof(score.player_name) - 1);
        score.player_name[sizeof(score.player_name) - 1] = '\0';
        
        score.score = getPlayerScore(client_id);
        score.enemies_killed = getPlayerKills(client_id);
        
        scores.push_back(score);
    }
    
    return scores;
}

void GameModule::scheduleNextEnemySpawn()
{
    _enemySpawnInterval = _enemySpawnIntervalDist(_rng);
}