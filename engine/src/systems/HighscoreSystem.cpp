#include "engine/systems/HighscoreSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Score.hpp"
#include <iostream>

HighscoreSystem::HighscoreSystem()
    : _scoreSaved(false)
{
}

void HighscoreSystem::update(Registry&, float)
{
}

bool HighscoreSystem::checkIfPlayerDead(Registry& registry) {
    bool playerDead = true;
    
    registry.each<Controllable, Health>([&playerDead](EntityID, Controllable&, Health& health) {
        if (health.current > 0.f) {
            playerDead = false;
        }
    });
    
    return playerDead;
}

void HighscoreSystem::saveCurrentScore(Registry& registry) {
    if (_scoreSaved) {
        return;
    }
    
    registry.each<Controllable, Score>([this](EntityID, Controllable&, Score& score) {
        uint32_t finalScore = score.getPoints();
        uint32_t finalKills = score.getEnemiesKilled();
        
        std::cout << "[Highscore] Final Score: " << finalScore 
                  << ", Final Kills: " << finalKills << std::endl;
        
        _manager.checkAndSave(finalScore, finalKills);
        _scoreSaved = true;
    });
}