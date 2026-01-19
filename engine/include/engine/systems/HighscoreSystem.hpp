#pragma once
#include "engine/core/System.hpp"
#include "engine/gameplay/HighscoreManager.hpp"

class HighscoreSystem : public System {
public:
    HighscoreSystem();
    
    void update(Registry&, float) override;
    void render(Registry&, Renderer&) override {}
    
    const ecs::HighscoreData& getHighscoreData() const { return _manager.getData(); }
    bool checkIfPlayerDead(Registry& registry);
    void saveCurrentScore(Registry& registry);
    bool isNewScoreRecord(uint32_t score) const { return _manager.isNewScoreRecord(score); }
    bool isNewKillsRecord(uint32_t kills) const { return _manager.isNewKillsRecord(kills); }
    void reset() { _scoreSaved = false; }
    
private:
    ecs::HighscoreManager _manager;
    bool _scoreSaved;
};