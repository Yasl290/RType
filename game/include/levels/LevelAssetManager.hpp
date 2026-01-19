#pragma once
#include "ILevelRenderer.hpp"
#include <memory>
#include <vector>

class LevelAssetManager {
private:
    std::vector<std::unique_ptr<ILevelRenderer>> _levels;
    size_t _currentLevelIndex;
    
public:
    LevelAssetManager();
    
    void addLevel(std::unique_ptr<ILevelRenderer> level);
    
    ILevelRenderer* getCurrentLevel();
    const ILevelRenderer* getCurrentLevel() const;
    
    bool nextLevel();
    void reset();
    
    size_t getCurrentLevelIndex() const { return _currentLevelIndex; }
};
