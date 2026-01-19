#pragma once

#include <cstdint>
#include "engine/core/Component.hpp"

class Score : public Component {
public:
    Score(uint32_t initialPoints = 0, uint32_t initialKills = 0);
    ~Score() override = default;
    
    void addPoints(uint32_t amount);
    void incrementKills();
    void reset();
    
    uint32_t getPoints() const;
    uint32_t getEnemiesKilled() const;

    
private:
    uint32_t _points;
    uint32_t _enemiesKilled;
};