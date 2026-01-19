#include "engine/systems/EnemySpawnSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/gameplay/EnemyFactory.hpp"
#include <random>

EnemySpawnSystem::EnemySpawnSystem(float windowWidth, float windowHeight)
    : _windowWidth(windowWidth),
      _timeSinceLastSpawn(0.f),
      _nextSpawnInterval(1.f),
      _rng(std::random_device{}()),
      _intervalDist(0.5f, 3.0f),
      _spawnYDist(50.f, windowHeight - 100.f)
{
    scheduleNextSpawn();
}

void EnemySpawnSystem::scheduleNextSpawn()
{
    _nextSpawnInterval = _intervalDist(_rng);
}

void EnemySpawnSystem::update(Registry& registry, float deltaTime)
{
    _timeSinceLastSpawn += deltaTime;

    if (_timeSinceLastSpawn >= _nextSpawnInterval) {
        float spawnX = _windowWidth + 150.f;
        float spawnY = _spawnYDist(_rng);

        createBasicEnemy(registry, spawnX, spawnY);

        _timeSinceLastSpawn = 0.f;
        scheduleNextSpawn();
    }
}