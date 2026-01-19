#pragma once

#include <engine/core/Registry.hpp>
#include <engine/gameplay/Enemy.hpp>
#include <random>
#include <functional>

struct EnemySpawnConfig {
    float spawnX;
    float spawnY;
    float velocityX;
    float velocityY;
    EnemyType type;
    float health;
};

struct BossSpawnConfig {
    float spawnX;
    float spawnY;
    float velocityX;
    float health;
    uint32_t scoreThreshold;
};

class ILevel {
public:
    virtual ~ILevel() = default;
    
    virtual const char* getName() const = 0;
    virtual void init(Registry& registry) = 0;
    virtual void update(float dt, Registry& registry) = 0;
    
    virtual bool shouldSpawnEnemy(float worldTime) = 0;
    virtual EnemySpawnConfig getEnemySpawn(std::mt19937& rng) = 0;
    
    virtual bool shouldSpawnBoss(uint32_t playerScore, bool bossAlreadySpawned) = 0;
    virtual BossSpawnConfig getBossSpawn() = 0;
    
    virtual float getEnemyShootInterval(EnemyType type) = 0;
    
    virtual void updateBossBehavior(
        float dt,
        EntityID bossId,
        Registry& registry,
        std::mt19937& rng,
        std::function<void(float, float, float, float)> spawnProjectile
    ) = 0;
    
    virtual bool isComplete(Registry& registry, bool bossSpawned) = 0;
};