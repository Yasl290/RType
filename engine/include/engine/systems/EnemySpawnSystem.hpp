#pragma once

#include "engine/core/System.hpp"
#include <random>

class EnemySpawnSystem : public System {
public:
    EnemySpawnSystem(float windowWidth, float windowHeight);

    void update(Registry& registry, float deltaTime) override;

private:
    float _windowWidth;

    float _timeSinceLastSpawn;
    float _nextSpawnInterval;

    std::mt19937 _rng;
    std::uniform_real_distribution<float> _intervalDist;
    std::uniform_real_distribution<float> _spawnYDist;

    void scheduleNextSpawn();
};