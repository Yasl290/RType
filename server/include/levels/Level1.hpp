#pragma once

#include "ILevel.hpp"
#include <random>

class Level1 : public ILevel {
public:
    Level1();
    
    const char* getName() const override { return "Level 1: The Beginning"; }
    void init(Registry& registry) override;
    void update(float dt, Registry& registry) override;
    
    bool shouldSpawnEnemy(float worldTime) override;
    EnemySpawnConfig getEnemySpawn(std::mt19937& rng) override;
    
    bool shouldSpawnBoss(uint32_t playerScore, bool bossAlreadySpawned) override;
    BossSpawnConfig getBossSpawn() override;
    
    float getEnemyShootInterval(EnemyType type) override;
    
    void updateBossBehavior(
        float dt,
        EntityID bossId,
        Registry& registry,
        std::mt19937& rng,
        std::function<void(float, float, float, float)> spawnProjectile
    ) override;
    
    bool isComplete(Registry& registry, bool bossSpawned) override;

private:
    static constexpr float WORLD_WIDTH = 1280.f;
    static constexpr float WORLD_HEIGHT = 720.f;
    static constexpr float ENEMY_SPEED = 120.f;
    static constexpr float BOSS_BULLET_SPEED = 250.f;
    static constexpr float ENEMY_SPAWN_MIN = 0.8f;
    static constexpr float ENEMY_SPAWN_MAX = 2.5f;
    static constexpr uint32_t BOSS_SPAWN_SCORE = 300;
    static constexpr float BOSS_MAX_HP = 10000.f;

    float _enemySpawnTimer;
    float _nextSpawnInterval;
    std::uniform_real_distribution<float> _spawnIntervalDist;
    std::uniform_real_distribution<float> _spawnYDist;
    
    bool _bossSpawned;
};