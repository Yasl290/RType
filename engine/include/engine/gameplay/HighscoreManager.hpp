#pragma once
#include <cstdint>
#include <string>

namespace ecs {

struct HighscoreData {
    uint32_t bestScore;
    uint32_t bestKills;
    
    HighscoreData() : bestScore(0), bestKills(0) {}
};

class HighscoreManager {
public:
    HighscoreManager();
    
    void checkAndSave(uint32_t currentScore, uint32_t currentKills);
    const HighscoreData& getData() const { return _data; }
    
    bool isNewScoreRecord(uint32_t score) const { return score > _data.bestScore; }
    bool isNewKillsRecord(uint32_t kills) const { return kills > _data.bestKills; }
    
private:
    void load();
    void save();
    
    std::string _filepath;
    HighscoreData _data;
};

}