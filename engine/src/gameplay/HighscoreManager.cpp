#include "engine/gameplay/HighscoreManager.hpp"
#include <fstream>
#include <iostream>

namespace ecs {

HighscoreManager::HighscoreManager()
    : _filepath("highscore.dat")
{
    load();
}

void HighscoreManager::checkAndSave(uint32_t currentScore, uint32_t currentKills) {
    bool updated = false;
    
    if (currentScore > _data.bestScore) {
        _data.bestScore = currentScore;
        updated = true;
        std::cout << "[Highscore] NEW SCORE RECORD! " << currentScore << std::endl;
    }
    
    if (currentKills > _data.bestKills) {
        _data.bestKills = currentKills;
        updated = true;
        std::cout << "[Highscore] NEW KILLS RECORD! " << currentKills << std::endl;
    }
    
    if (updated) {
        save();
    }
}

void HighscoreManager::load() {
    std::ifstream file(_filepath, std::ios::binary);
    
    if (!file.is_open()) {
        std::cout << "[Highscore] No save file found, starting fresh" << std::endl;
        return;
    }
    
    file.read(reinterpret_cast<char*>(&_data.bestScore), sizeof(_data.bestScore));
    file.read(reinterpret_cast<char*>(&_data.bestKills), sizeof(_data.bestKills));
    
    file.close();
    
    std::cout << "[Highscore] Loaded - Best Score: " << _data.bestScore 
              << ", Best Kills: " << _data.bestKills << std::endl;
}

void HighscoreManager::save() {
    std::ofstream file(_filepath, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "[Highscore] Failed to save!" << std::endl;
        return;
    }
    
    file.write(reinterpret_cast<const char*>(&_data.bestScore), sizeof(_data.bestScore));
    file.write(reinterpret_cast<const char*>(&_data.bestKills), sizeof(_data.bestKills));
    
    file.close();
}

} // namespace ecs