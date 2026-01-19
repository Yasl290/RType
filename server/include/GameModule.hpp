#pragma once
#include "engine/core/Registry.hpp"
#include "engine/systems/MovementSystem.hpp"
#include "levels/LevelManager.hpp"
#include "protocol/Protocol.hpp"
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <shared_mutex>
#include <unordered_map>

enum class EventType {
  ENTITY_SPAWNED,
  ENTITY_DESTROYED,
  ENTITY_FIRED,
  PLAYER_DIED
};

struct LocalGameEvent {
  EventType type;
  uint32_t entity_id;
  uint32_t related_id;
  uint8_t entity_type;
  uint32_t extra_data;
  float pos_x;
  float pos_y;
};

struct EntitySnapshot {
  uint32_t entity_id;
  float pos_x;
  float pos_y;
  float vel_x;
  float vel_y;
  uint8_t entity_type;
  float hp_current;
  float hp_max;
  uint8_t player_slot;
};

struct PlayerState {
  bool isCharging = false;
  float chargeTime = 0.f;
  uint8_t slot = 0;
  bool doubleDamage = false;
  bool invincibility = false;
  bool doubleFireRate = false;
};

class GameModule {
private:
  static constexpr size_t MAX_PLAYERS = 4;
  static constexpr float WORLD_WIDTH = 1280.f;
  static constexpr float WORLD_HEIGHT = 720.f;
  static constexpr float CHARGED_SHOT_THRESHOLD = 1.0f;
  static constexpr float MAX_CHARGE_TIME = 2.0f;
  static constexpr float NORMAL_SHOT_COOLDOWN = 0.2f;
  static constexpr float CHARGED_SHOT_COOLDOWN = 1.5f;
  static constexpr float PROJECTILE_MARGIN = 100.f;
  static constexpr float ENEMY_SPEED = 150.f;
  static constexpr float ENEMY_KILL_POINTS = 100.f;
  static constexpr float ENEMY_CONTACT_DAMAGE = 50.f;
  static constexpr float DESPAWN_TIMEOUT = 0.1f;
  static constexpr float BOSS_TARGET_X = 800.f;
  static constexpr float BOSS_W = 200.f;
  static constexpr float BOSS_H = 200.f;

  Registry _registry;
  std::unique_ptr<MovementSystem> _movementSystem;
  std::unique_ptr<LevelManager> _levelManager;

  mutable std::shared_mutex _registryMutex;
  mutable std::mutex _playerMutex;
  mutable std::mutex _eventMutex;
  mutable std::mutex _scoreMutex;

  std::unordered_map<uint32_t, EntityID> _playerEntities;
  std::unordered_map<uint32_t, PlayerState> _playerStates;
  std::queue<LocalGameEvent> _eventQueue;
  std::queue<RType::Protocol::GameEvent> _networkEventQueue;

  float _enemySpawnTimer;
  float _enemySpawnInterval;
  std::mt19937 _rng;
  std::uniform_real_distribution<float> _enemySpawnIntervalDist;
  std::uniform_real_distribution<float> _enemySpawnYDist;
  std::unordered_map<EntityID, float> _enemyShootTimers;

  bool _bossSpawned;
  EntityID _bossEntity;

  std::unordered_map<uint32_t, uint32_t> _persistentScores;
  std::unordered_map<uint32_t, uint32_t> _persistentKills;
  bool _inLevelTransition;
  float _transitionTimer;
  static constexpr float TRANSITION_DURATION = 3.0f;
  std::string _pendingLevelName;

  bool _gameOverFlag;

public:
  GameModule();

  void init();
  void update(float dt);

  std::vector<EntitySnapshot> getWorldSnapshot();
  std::vector<LocalGameEvent> pollEvents();
  std::vector<RType::Protocol::GameEvent> pollNetworkEvents();

  uint32_t spawnPlayer(uint32_t client_id, float x, float y, uint8_t player_slot);
  void removePlayer(uint32_t client_id);
  void processInput(uint32_t client_id, uint8_t flags);

  uint32_t getPlayerScore(uint32_t client_id);
  uint32_t getPlayerKills(uint32_t client_id);
  void savePlayerScore(uint32_t client_id);

  bool areAllPlayersDead();
  std::vector<RType::Protocol::PlayerFinalScore> getFinalScores();

  bool nextLevel();
  const char* getCurrentLevelName() const;

  bool isGameOver() const { return _gameOverFlag; }

private:
  void cleanupProjectiles();
  void spawnProjectile(float x, float y, bool charged, uint32_t owner_id);
  void spawnEnemyProjectile(float x, float y, float vx, float vy, float damage = 10.f);
  void updateEnemies(float dt, ILevel* level);
  void spawnEnemy(const EnemySpawnConfig& config);
  void spawnBoss(const BossSpawnConfig& config);
  void handleEnemyCollisions();
  void scheduleNextEnemySpawn();
  void handleShootRelease(uint32_t client_id, PlayerState& state);
  void updateChargeStates(float dt);
};