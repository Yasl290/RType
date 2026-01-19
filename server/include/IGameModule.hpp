#pragma once

#include <cstdint>
#include <vector>

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

class IGameModule {
public:
    virtual ~IGameModule() = default;
    
    virtual void init() = 0;
    virtual void update(float dt) = 0;
    
    virtual uint32_t spawnPlayer(uint32_t client_id, float x, float y, uint8_t player_slot) = 0;
    virtual void removePlayer(uint32_t client_id) = 0;
    
    virtual void processInput(uint32_t client_id, uint8_t input_flags) = 0;
    
    virtual std::vector<EntitySnapshot> getWorldSnapshot() = 0;
};