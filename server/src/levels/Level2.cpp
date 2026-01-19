#include "levels/Level2.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/Enemy.hpp"
#include <cmath>
#include <iostream>

Level2::Level2()
    : _enemySpawnTimer(0.f)
    , _nextSpawnInterval(2.f)
    , _spawnIntervalDist(ENEMY_SPAWN_MIN, ENEMY_SPAWN_MAX)
    , _spawnYDist(50.f, WORLD_HEIGHT - 100.f)
    , _bossSpawned(false)
{
}

void Level2::init(Registry&)
{
    std::mt19937 rng{std::random_device{}()};
    _enemySpawnTimer = 0.f;
    _nextSpawnInterval = _spawnIntervalDist(rng);
    _bossSpawned = false;
    std::cout << "[Level2] Initialized" << std::endl;
}

void Level2::update(float dt, Registry&)
{
    _enemySpawnTimer += dt;
}

bool Level2::shouldSpawnEnemy(float)
{
    if (_enemySpawnTimer >= _nextSpawnInterval) {
        _enemySpawnTimer = 0.f;
        return true;
    }
    return false;
}

EnemySpawnConfig Level2::getEnemySpawn(std::mt19937& rng)
{
    _nextSpawnInterval = _spawnIntervalDist(rng);
    
    std::uniform_int_distribution<int> typeDist(0, 2);
    int typeRoll = typeDist(rng);
    
    EnemySpawnConfig config;
    config.spawnX = WORLD_WIDTH + 150.f;
    config.spawnY = _spawnYDist(rng);
    
    if (typeRoll == 0) {
        config.type = EnemyType::Basic;
        config.velocityX = -ENEMY_SPEED;
        config.velocityY = 0.f;
        config.health = 60.f;
    } else if (typeRoll == 1) {
        config.type = EnemyType::FastShooter;
        config.velocityX = -ENEMY_SPEED * 1.2f;
        config.velocityY = 0.f;
        config.health = 40.f;
    } else {
        config.type = EnemyType::Bomber;
        config.velocityX = -ENEMY_SPEED * 0.7f;
        config.velocityY = 0.f;
        config.health = 80.f;
    }
    
    return config;
}

bool Level2::shouldSpawnBoss(uint32_t playerScore, bool bossAlreadySpawned)
{
    return !bossAlreadySpawned && playerScore >= BOSS_SPAWN_SCORE;
}

BossSpawnConfig Level2::getBossSpawn()
{
    BossSpawnConfig config;
    config.spawnX = WORLD_WIDTH + 200.f;
    config.spawnY = (WORLD_HEIGHT * 0.42f) - (200.f * 0.5f);
    config.velocityX = -ENEMY_SPEED * 0.5f;
    config.health = BOSS_MAX_HP;
    config.scoreThreshold = BOSS_SPAWN_SCORE;
    
    return config;
}

float Level2::getEnemyShootInterval(EnemyType type)
{
    switch (type) {
        case EnemyType::Boss:
            return 0.f;
        case EnemyType::FastShooter:
            return 1.0f;
        case EnemyType::Bomber:
            return 3.0f;
        case EnemyType::Basic:
        default:
            return 2.0f;
    }
}

void Level2::updateBossBehavior(float, EntityID bossId, Registry& registry,
                                std::mt19937& rng,
                                std::function<void(float, float, float, float)> spawnProjectile) {

    if (!registry.has<Health>(bossId) || !registry.has<Transform>(bossId)) {
        return;
    }
    
    const Health& bh = registry.get<Health>(bossId);
    const Transform& bt = registry.get<Transform>(bossId);
    
    float centerX = bt.x + 200.f * 0.5f;
    float centerY = bt.y + 200.f * 0.5f;
    
    if (bh.max <= 0.f) return;
    float ratio = bh.current / bh.max;
    
    const float PI = 3.14159265f;
    const float DEG_TO_RAD = PI / 180.f;
    
    if (ratio >= 0.66f) {
        static float spiralAngle = 0.f;
        spiralAngle += 18.f;
        if (spiralAngle >= 360.f) spiralAngle -= 360.f;
        
        for (int i = 0; i < 5; ++i) {
            float angle = (spiralAngle + i * 72.f) * DEG_TO_RAD;
            float vx = -BOSS_BULLET_SPEED * std::cos(angle);
            float vy = BOSS_BULLET_SPEED * std::sin(angle);
            spawnProjectile(centerX, centerY, vx, vy);
        }
    }
    
    else if (ratio >= 0.33f) {
        static bool phase2Toggle = false;
        
        if (!phase2Toggle) {
            bool foundPlayer = false;
            float targetX = centerX - 100.f;
            float targetY = centerY;
            
            registry.each<Transform, Controllable>([&](EntityID, Transform& pt, Controllable&) {
                if (!foundPlayer) {
                    targetX = pt.x;
                    targetY = pt.y;
                    foundPlayer = true;
                }
            });
            
            for (int i = -1; i <= 1; ++i) {
                float dx = targetX - centerX;
                float dy = (targetY - centerY) + (i * 40.f);
                float len = std::sqrt(dx * dx + dy * dy);
                
                if (len > 0.001f) {
                    float vx = (dx / len) * BOSS_BULLET_SPEED * 1.2f;
                    float vy = (dy / len) * BOSS_BULLET_SPEED * 1.2f;
                    spawnProjectile(centerX, centerY, vx, vy);
                }
            }
        } else {
            const float angles[5] = {-30.f, -15.f, 0.f, 15.f, 30.f};
            for (float angleDeg : angles) {
                float rad = angleDeg * DEG_TO_RAD;
                float vx = -BOSS_BULLET_SPEED * std::cos(rad);
                float vy = BOSS_BULLET_SPEED * std::sin(rad);
                spawnProjectile(centerX, centerY, vx, vy);
            }
        }
        
        phase2Toggle = !phase2Toggle;
    }
    
    else {
        static int phase3Counter = 0;
        static float phase3Speed = BOSS_BULLET_SPEED;
        phase3Counter++;
        
        phase3Speed = std::min(phase3Speed + 1.5f, BOSS_BULLET_SPEED * 1.5f);
        
        int pattern = (phase3Counter / 3) % 3;
        
        if (pattern == 0) {
            std::uniform_real_distribution<float> angleDist(-60.f, 60.f);
            for (int i = 0; i < 5; ++i) {
                float angle = angleDist(rng);
                float rad = angle * DEG_TO_RAD;
                float vx = -phase3Speed * std::cos(rad);
                float vy = phase3Speed * std::sin(rad);
                spawnProjectile(centerX, centerY, vx, vy);
            }
            
        } else if (pattern == 1) {
            for (int i = 0; i < 6; ++i) {
                float angle = (i * 60.f) * DEG_TO_RAD;
                float vx = -phase3Speed * std::cos(angle);
                float vy = phase3Speed * std::sin(angle);
                spawnProjectile(centerX, centerY, vx, vy);
            }
            
            std::uniform_real_distribution<float> xDist(WORLD_WIDTH * 0.6f, WORLD_WIDTH);
            float x = xDist(rng);
            spawnProjectile(x, -20.f, -phase3Speed * 0.3f, phase3Speed * 0.7f);
            
        } else {
            static float chaosAngle = 0.f;
            chaosAngle += 20.f;
            if (chaosAngle >= 360.f) chaosAngle -= 360.f;
            
            for (int i = 0; i < 5; ++i) {
                float angle = (chaosAngle + i * 72.f) * DEG_TO_RAD;
                float vx = -phase3Speed * std::cos(angle);
                float vy = phase3Speed * std::sin(angle);
                spawnProjectile(centerX, centerY, vx, vy);
            }
            
            bool foundPlayer = false;
            float targetX = centerX - 100.f;
            float targetY = centerY;
            
            registry.each<Transform, Controllable>([&](EntityID, Transform& pt, Controllable&) {
                if (!foundPlayer) {
                    targetX = pt.x;
                    targetY = pt.y;
                    foundPlayer = true;
                }
            });
            
            float dx = targetX - centerX;
            float dy = targetY - centerY;
            float len = std::sqrt(dx * dx + dy * dy);
            
            if (len > 0.001f) {
                float vx = (dx / len) * phase3Speed * 1.3f;
                float vy = (dy / len) * phase3Speed * 1.3f;
                spawnProjectile(centerX, centerY, vx, vy);
            }
        }
    }
}

bool Level2::isComplete(Registry& registry, bool bossSpawned)
{
    if (!bossSpawned) {
        return false;
    }
    
    bool bossAlive = false;
    registry.each<Enemy>([&](EntityID, Enemy& e) {
        if (e.type == EnemyType::Boss) {
            bossAlive = true;
        }
    });
    
    return !bossAlive;
}