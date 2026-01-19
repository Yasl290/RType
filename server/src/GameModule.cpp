#include "GameModule.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/Projectile.hpp"
#include "engine/gameplay/Enemy.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Score.hpp"
#include "protocol/Protocol.hpp"
#include "levels/LevelManager.hpp"
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>

namespace {
    inline bool rectsOverlap(float x1, float y1, float w1, float h1,
                             float x2, float y2, float w2, float h2)
    {
        return x1 < x2 + w2 && x1 + w1 > x2 &&
               y1 < y2 + h2 && y1 + h1 > y2;
    }
}

GameModule::GameModule()
    : _enemySpawnTimer(0.f)
    , _enemySpawnInterval(1.f)
    , _rng(std::random_device{}())
    , _enemySpawnIntervalDist(0.5f, 3.0f)
    , _enemySpawnYDist(50.f, WORLD_HEIGHT - 100.f)
    , _bossSpawned(false)
    , _bossEntity(0)
    , _levelManager(std::make_unique<LevelManager>())
    , _inLevelTransition(false)
    , _transitionTimer(0.f)
    , _gameOverFlag(false)
{
    _movementSystem = std::make_unique<MovementSystem>();
}


void GameModule::init()
{
    std::unique_lock<std::shared_mutex> regLock(_registryMutex);
    std::lock_guard<std::mutex> playerLock(_playerMutex);
    std::lock_guard<std::mutex> eventLock(_eventMutex);
    std::lock_guard<std::mutex> scoreLock(_scoreMutex);
    
    _playerStates.clear();
    _playerEntities.clear();
    _movementSystem = std::make_unique<MovementSystem>();

    _enemySpawnTimer = 0.f;
    scheduleNextEnemySpawn();
    _enemyShootTimers.clear();

    _bossSpawned = false;
    _bossEntity = 0;
    _inLevelTransition = false;
    _transitionTimer = 0.f;
    _gameOverFlag = false;
    
    _persistentScores.clear();
    _persistentKills.clear();
    
    while (!_eventQueue.empty()) {
        _eventQueue.pop();
    }
    
    _registry.each<Transform>([&](EntityID id, Transform&) {
        _registry.markForDestruction(id);
    });
    _registry.cleanup();
    
    _levelManager->reset();
    if (auto* level = _levelManager->getCurrentLevel()) {
        level->init(_registry);
    }
    
    std::cout << "[GameModule] Fully reset - all entities cleared" << std::endl;
}

void GameModule::update(float dt)
{
    std::unique_lock<std::shared_mutex> lock(_registryMutex);
    
    auto* currentLevel = _levelManager->getCurrentLevel();
    if (!currentLevel) return;

    if (_inLevelTransition) {
        _transitionTimer += dt;
        
        if (_transitionTimer >= TRANSITION_DURATION) {
            std::cout << "[GameModule] Transition complete, starting " 
                      << _pendingLevelName << std::endl;

            _inLevelTransition = false;
            _transitionTimer = 0.f;
            _bossSpawned = false;
            _bossEntity = 0;

            std::vector<EntityID> toDestroy;
            _registry.each<Transform>([&](EntityID id, Transform&) {
                if (_registry.has<Controllable>(id)) {
                    if (_registry.has<Score>(id)) {
                        Score& score = _registry.get<Score>(id);
                        score = Score(0, 0);
                    }
                    
                    if (_registry.has<Health>(id)) {
                        Health& health = _registry.get<Health>(id);
                        health.current = 100.f;
                        health.max = 100.f;
                    }
                    Transform& t = _registry.get<Transform>(id);

                    uint8_t slot = 0;
                    for (const auto& [client_id, entity_id] : _playerEntities) {
                        if (entity_id == id) {
                            auto stateIt = _playerStates.find(client_id);
                            if (stateIt != _playerStates.end()) {
                                slot = stateIt->second.slot;
                            }
                            break;
                        }
                    }
                    
                    t.x = 100.f;
                    t.y = 150.f + (slot * 120.f);
                    
                    if (_registry.has<Velocity>(id)) {
                        Velocity& v = _registry.get<Velocity>(id);
                        v.x = 0.f;
                        v.y = 0.f;
                    }
                    
                    return;
                }

                toDestroy.push_back(id);
            });

            for (EntityID id : toDestroy) {
                _registry.markForDestruction(id);
            }
            _registry.cleanup();

            _enemyShootTimers.clear();
  
            _persistentScores.clear();
            _persistentKills.clear();

            currentLevel->init(_registry);
            
            std::cout << "[GameModule] Level " << _pendingLevelName 
                      << " started with players preserved" << std::endl;
        }

        return;
    }

    currentLevel->update(dt, _registry);
    _movementSystem->update(_registry, dt);

    _registry.each<Transform, Controllable>([](EntityID, Transform& t, Controllable&) {
        constexpr float PLAYER_WIDTH = 100.f;
        constexpr float PLAYER_HEIGHT = 60.f;

        if (t.x < 0.f) t.x = 0.f;
        if (t.x + PLAYER_WIDTH > WORLD_WIDTH) t.x = WORLD_WIDTH - PLAYER_WIDTH;
        if (t.y < 0.f) t.y = 0.f;
        if (t.y + PLAYER_HEIGHT > WORLD_HEIGHT) t.y = WORLD_HEIGHT - PLAYER_HEIGHT;
    });

    _registry.each<Controllable>([dt](EntityID, Controllable& c) {
        if (!c.canShoot) {
            c.currentCooldown -= dt;
            if (c.currentCooldown <= 0.f) {
                c.canShoot = true;
                c.currentCooldown = 0.f;
            }
        }
    });

    if (!_bossSpawned) {
        for (const auto& [client_id, entity_id] : _playerEntities) {
            if (_registry.has<Score>(entity_id)) {
                const Score& s = _registry.get<Score>(entity_id);
                if (currentLevel->shouldSpawnBoss(s.getPoints(), _bossSpawned)) {
                    std::cout << "[GameModule] *** BOSS SPAWN TRIGGERED *** Player " << client_id
                              << " score: " << s.getPoints() << std::endl;
                    _bossSpawned = true;
                    auto config = currentLevel->getBossSpawn();
                    spawnBoss(config);
                    break;
                }
            }
        }
    }

    updateChargeStates(dt);
    updateEnemies(dt, currentLevel);
    handleEnemyCollisions();
    cleanupProjectiles();
    _registry.cleanup();

    if (currentLevel->isComplete(_registry, _bossSpawned)) {
        std::cout << "[GameModule] Level complete!" << std::endl;
        
        const char* currentLevelName = currentLevel->getName();
        
        if (!_levelManager->hasNextLevel()) {
            std::cout << "[GameModule] Last level completed! Triggering GAME OVER!" << std::endl;
            _gameOverFlag = true;
        } else {
            if (_levelManager->nextLevel()) {
                auto* nextLevel = _levelManager->getCurrentLevel();
                const char* nextLevelName = nextLevel ? nextLevel->getName() : "Unknown";
                
                std::cout << "[GameModule] Starting transition: " 
                          << currentLevelName << " -> " << nextLevelName << std::endl;
                
                {
                    std::lock_guard<std::mutex> eventLock(_eventMutex);
                    RType::Protocol::GameEvent evt;
                    evt.event_type = RType::Protocol::GameEventType::LEVEL_COMPLETE;
                    evt.entityId = 0;
                    evt.killerId = 0;
                    evt.scoreGain = 0;
                    std::strncpy(evt.levelName, currentLevelName, 63);
                    evt.levelName[63] = '\0';
                    std::strncpy(evt.nextLevelName, nextLevelName, 63);
                    evt.nextLevelName[63] = '\0';
                    _networkEventQueue.push(evt);
                }

                _inLevelTransition = true;
                _transitionTimer = 0.f;
                _pendingLevelName = nextLevelName;
            }
        }
    }
}

std::vector<EntitySnapshot> GameModule::getWorldSnapshot()
{
    std::shared_lock<std::shared_mutex> regLock(_registryMutex);
    
    std::vector<EntitySnapshot> snapshots;

    std::unordered_map<EntityID, uint32_t> entityToClient;
    {
        std::lock_guard<std::mutex> playerLock(_playerMutex);
        for (const auto& [client_id, entity_id] : _playerEntities) {
            entityToClient[entity_id] = client_id;
        }
    }

    _registry.each<Transform>([&](EntityID id, const Transform& transform) {
        EntitySnapshot snap;
        auto it = entityToClient.find(id);
        if (it != entityToClient.end()) {
            snap.entity_id = it->second;
            std::lock_guard<std::mutex> playerLock(_playerMutex);
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
            const Enemy& e = _registry.get<Enemy>(id);
            switch (e.type) {
                case EnemyType::Boss:
                    snap.entity_type = 5;
                    break;
                case EnemyType::FastShooter:
                    snap.entity_type = 6;
                    break;
                case EnemyType::Bomber:
                    snap.entity_type = 7;
                    break;
                case EnemyType::Basic:
                default:
                    snap.entity_type = 1;
                    break;
            }
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

std::vector<LocalGameEvent> GameModule::pollEvents()
{
    std::lock_guard<std::mutex> lock(_eventMutex);
    std::vector<LocalGameEvent> events;
    while (!_eventQueue.empty()) {
        events.push_back(_eventQueue.front());
        _eventQueue.pop();
    }
    return events;
}

std::vector<RType::Protocol::GameEvent> GameModule::pollNetworkEvents()
{
    std::lock_guard<std::mutex> lock(_eventMutex);
    std::vector<RType::Protocol::GameEvent> events;
    while (!_networkEventQueue.empty()) {
        events.push_back(_networkEventQueue.front());
        _networkEventQueue.pop();
    }
    return events;
}

bool GameModule::nextLevel()
{
    return _levelManager->nextLevel();
}

const char* GameModule::getCurrentLevelName() const
{
    if (auto* level = _levelManager->getCurrentLevel()) {
        return level->getName();
    }
    return "Unknown";
}