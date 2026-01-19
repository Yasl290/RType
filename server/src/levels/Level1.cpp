#include "levels/Level1.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/Enemy.hpp"
#include <cmath>
#include <iostream>

Level1::Level1()
    : _enemySpawnTimer(0.f)
    , _nextSpawnInterval(2.f)
    , _spawnIntervalDist(ENEMY_SPAWN_MIN, ENEMY_SPAWN_MAX)
    , _spawnYDist(50.f, WORLD_HEIGHT - 100.f)
    , _bossSpawned(false)
{
}

void Level1::init(Registry&)
{
    std::mt19937 rng{std::random_device{}()};
    _enemySpawnTimer = 0.f;
    _nextSpawnInterval = _spawnIntervalDist(rng);
    _bossSpawned = false;
    std::cout << "[Level1] Initialized" << std::endl;
}

void Level1::update(float dt, Registry&)
{
    _enemySpawnTimer += dt;
}

bool Level1::shouldSpawnEnemy(float)
{
    if (_enemySpawnTimer >= _nextSpawnInterval) {
        _enemySpawnTimer = 0.f;
        return true;
    }
    return false;
}

EnemySpawnConfig Level1::getEnemySpawn(std::mt19937& rng)
{
    _nextSpawnInterval = _spawnIntervalDist(rng);
    
    EnemySpawnConfig config;
    config.spawnX = WORLD_WIDTH + 150.f;
    config.spawnY = _spawnYDist(rng);
    config.velocityX = -ENEMY_SPEED;
    config.velocityY = 0.f;
    config.type = EnemyType::Basic;
    config.health = 50.f;
    
    return config;
}

bool Level1::shouldSpawnBoss(uint32_t playerScore, bool bossAlreadySpawned)
{
    if (!bossAlreadySpawned && playerScore >= BOSS_SPAWN_SCORE) {
        _bossSpawned = true;
        return true;
    }
    return false;
}

BossSpawnConfig Level1::getBossSpawn()
{
    BossSpawnConfig config;
    config.spawnX = WORLD_WIDTH + 200.f;
    config.spawnY = (WORLD_HEIGHT * 0.42f) - (200.f * 0.5f);
    config.velocityX = -ENEMY_SPEED * 0.5f;
    config.health = BOSS_MAX_HP;
    config.scoreThreshold = BOSS_SPAWN_SCORE;
    
    return config;
}

float Level1::getEnemyShootInterval(EnemyType type)
{
    if (type == EnemyType::Boss) {
        return 0.f;
    }
    return 2.5f;
}

void Level1::updateBossBehavior(float, EntityID bossId, Registry& registry,
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
    const float DEG_TO_RAD = 1.0f / 180.f;
    const float degToRad = PI * DEG_TO_RAD;
    
    if (ratio >= 0.66f) {
        const float anglesDeg[5] = { -20.f, -10.f, 0.f, 10.f, 20.f };
        for (float aDeg : anglesDeg) {
            float rad = aDeg * degToRad;
            float vx = -BOSS_BULLET_SPEED * std::cos(rad);
            float vy = BOSS_BULLET_SPEED * std::sin(rad);
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
            
            float dx = targetX - centerX;
            float dy = targetY - centerY;
            float len = std::sqrt(dx * dx + dy * dy);
            
            float vx = -BOSS_BULLET_SPEED;
            float vy = 0.f;
            if (len > 0.001f) {
                vx = (dx / len) * BOSS_BULLET_SPEED;
                vy = (dy / len) * BOSS_BULLET_SPEED;
            }
            
            spawnProjectile(centerX, centerY, vx, vy);
        } else {
            const int N = 10;
            float step = WORLD_HEIGHT / static_cast<float>(N + 1);
            std::uniform_int_distribution<int> dist(0, N - 1);
            int hole = dist(rng);
            
            for (int i = 0; i < N; ++i) {
                if (i == hole) continue;
                float y = step * static_cast<float>(i + 1);
                spawnProjectile(centerX, y, -BOSS_BULLET_SPEED, 0.f);
            }
        }
        
        phase2Toggle = !phase2Toggle;
    }
    else {
        static float phase3ConeSpeed = BOSS_BULLET_SPEED;
        static float phase3RainSpeed = BOSS_BULLET_SPEED * 0.35f;
        static const float phase3ConeSpeedMax = BOSS_BULLET_SPEED * 1.7f;
        static const float phase3RainSpeedMax = BOSS_BULLET_SPEED * 0.65f;
        
        phase3ConeSpeed = std::min(phase3ConeSpeed + 5.f, phase3ConeSpeedMax);
        phase3RainSpeed = std::min(phase3RainSpeed + 1.5f, phase3RainSpeedMax);
        
        std::uniform_real_distribution<float> offsetDist(-60.f, 60.f);
        float offsetDeg = offsetDist(rng);
        float rad = offsetDeg * degToRad;
        
        float vxCone = -std::cos(rad) * phase3ConeSpeed;
        float vyCone = std::sin(rad) * phase3ConeSpeed;
        spawnProjectile(centerX, centerY, vxCone, vyCone);
        
        const int N = 1;
        float maxRainX = WORLD_WIDTH * 0.8f;
        std::uniform_real_distribution<float> spawnXDist(0.f, maxRainX);
        
        for (int i = 0; i < N; ++i) {
            float spawnX = spawnXDist(rng);
            float spawnY = -20.f;
            float vxRain = -40.f;
            float vyRain = phase3RainSpeed;
            spawnProjectile(spawnX, spawnY, vxRain, vyRain);
        }
    }
}

bool Level1::isComplete(Registry& registry, bool bossSpawned)
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