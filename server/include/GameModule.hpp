#pragma once

#include "IGameModule.hpp"
#include "protocol/Protocol.hpp" 
#include "engine/core/Registry.hpp"
#include "engine/systems/MovementSystem.hpp"
#include "engine/gameplay/Enemy.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Score.hpp"
#include <memory>
#include <unordered_map>
#include <random>
#include <queue>

enum class EventType : uint8_t {
    ENTITY_SPAWNED,
    ENTITY_DESTROYED,
    ENTITY_FIRED,
    PLAYER_DIED
};

struct GameEvent {
    EventType type;
    uint32_t entity_id;
    uint32_t related_id;
    float pos_x;
    float pos_y;
    uint8_t entity_type;
    uint8_t extra_data;
};

class GameModule : public IGameModule {
public:
    GameModule();
    ~GameModule() override = default;
    
    void init() override;
    void update(float dt) override;
    
    uint32_t spawnPlayer(uint32_t client_id, float x, float y, uint8_t player_slot) override;
    void removePlayer(uint32_t client_id) override;
    
    void processInput(uint32_t client_id, uint8_t input_flags) override;
    std::vector<EntitySnapshot> getWorldSnapshot() override;
    std::vector<GameEvent> pollEvents();
    uint32_t getPlayerScore(uint32_t client_id);
    uint32_t getPlayerKills(uint32_t client_id);
    bool areAllPlayersDead();
    std::vector<RType::Protocol::PlayerFinalScore> getFinalScores();
    
private:
    struct PlayerState {
        bool isCharging = false;
        float chargeTime = 0.f;
        uint8_t slot = 0;
    };

    Registry _registry;
    std::unique_ptr<MovementSystem> _movementSystem;
    std::unordered_map<uint32_t, EntityID> _playerEntities;
    std::unordered_map<uint32_t, PlayerState> _playerStates;
    
    std::unordered_map<uint32_t, uint32_t> _persistentScores;
    std::unordered_map<uint32_t, uint32_t> _persistentKills;
    
    std::queue<GameEvent> _eventQueue;

    void handleShootRelease(uint32_t client_id, PlayerState& state);
    void updateChargeStates(float dt);
    void cleanupProjectiles();
    void spawnProjectile(float x, float y, bool charged, uint32_t owner_id);
    void spawnEnemyProjectile(float x, float y);

    void updateEnemies(float dt);
    void spawnEnemy();
    void handleEnemyCollisions();
    void scheduleNextEnemySpawn();
    void savePlayerScore(uint32_t client_id);

    float _enemySpawnTimer;
    float _enemySpawnInterval;
    std::mt19937 _rng;
    std::uniform_real_distribution<float> _enemySpawnIntervalDist;
    std::uniform_real_distribution<float> _enemySpawnYDist;
    
    std::unordered_map<EntityID, float> _enemyShootTimers;
    
    static constexpr size_t MAX_PLAYERS = 4;
    static constexpr float WORLD_WIDTH = 1600.f;
    static constexpr float WORLD_HEIGHT = 720.f;
    static constexpr float PROJECTILE_MARGIN = 100.f;
    static constexpr float CHARGED_SHOT_THRESHOLD = 2.0f;
    static constexpr float MAX_CHARGE_TIME = 2.0f;
    static constexpr float NORMAL_SHOT_COOLDOWN = 0.25f;
    static constexpr float CHARGED_SHOT_COOLDOWN = 1.5f;

    static constexpr float ENEMY_SPEED = 150.f;
    static constexpr float ENEMY_CONTACT_DAMAGE = 20.f;
    static constexpr float ENEMY_SHOOT_INTERVAL = 2.0f;

    static constexpr uint32_t ENEMY_KILL_POINTS = 100;
};