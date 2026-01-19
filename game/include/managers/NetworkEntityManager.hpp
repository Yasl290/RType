#pragma once

#include "NetworkClient.hpp"
#include <engine/core/Entity.hpp>
#include <engine/core/Registry.hpp>
#include <unordered_map>
#include <cstdint>

class ILevelRenderer;
enum class EnemyType;

struct EntityInfo {
    EntityID id;
    float lastSeen;
};

class NetworkEntityManager {
public:
    explicit NetworkEntityManager(Registry& registry);

    void update(float dt, NetworkClient& network);
    void clearAll();

    void setLevelRenderer(ILevelRenderer* renderer) {
        _levelRenderer = renderer;
    }

    EntityID getLocalPlayerEntity(uint32_t networkId) const {
        auto it = _networkEntities.find(networkId);
        if (it != _networkEntities.end()) {
            return it->second.id;
        }
        return -1;
    }

private:
    void spawnEntity(const ReceivedEntity& update);
    void updateEntity(const ReceivedEntity& update, EntityID localEntity);
    void despawnTimedOutEntities(float dt);
    void spawnEnemyOfType(EntityID localEntity,
                          const ReceivedEntity& update,
                          EnemyType type,
                          float scale);
    const char* getEnemyTypeName(EnemyType type) const;

    Registry& _registry;
    std::unordered_map<uint32_t, EntityInfo> _networkEntities;
    ILevelRenderer* _levelRenderer;
};