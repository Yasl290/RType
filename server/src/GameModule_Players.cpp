#include "GameModule.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Score.hpp"
#include <iostream>
#include <algorithm>
#include <string>

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

    LocalGameEvent event;
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

    state.doubleDamage = (flags & (1 << 5)) != 0;
    state.invincibility = (flags & (1 << 6)) != 0;
    state.doubleFireRate = (flags & (1 << 7)) != 0;

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
    float cooldownMultiplier = state.doubleFireRate ? 0.5f : 1.0f;
    controllable.currentCooldown = (charged ? CHARGED_SHOT_COOLDOWN : NORMAL_SHOT_COOLDOWN) * cooldownMultiplier;
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