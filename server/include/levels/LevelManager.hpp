#pragma once

#include "ILevel.hpp"
#include <vector>
#include <memory>

class LevelManager {
public:
    LevelManager();
    
    void addLevel(std::unique_ptr<ILevel> level);
    
    ILevel* getCurrentLevel();
    const ILevel* getCurrentLevel() const;
    
    bool nextLevel();
    bool hasNextLevel() const;
    void reset();

private:
    std::vector<std::unique_ptr<ILevel>> _levels;
    size_t _currentLevelIndex;
};