#include "levels/LevelManager.hpp"
#include "levels/Level1.hpp"
#include "levels/Level2.hpp"

LevelManager::LevelManager()
    : _currentLevelIndex(0)
{
    addLevel(std::make_unique<Level1>());
    addLevel(std::make_unique<Level2>());
}

void LevelManager::addLevel(std::unique_ptr<ILevel> level)
{
    _levels.push_back(std::move(level));
}

ILevel* LevelManager::getCurrentLevel()
{
    if (_currentLevelIndex < _levels.size()) {
        return _levels[_currentLevelIndex].get();
    }
    return nullptr;
}

const ILevel* LevelManager::getCurrentLevel() const
{
    if (_currentLevelIndex < _levels.size()) {
        return _levels[_currentLevelIndex].get();
    }
    return nullptr;
}

bool LevelManager::nextLevel() {
    if (_currentLevelIndex + 1 < _levels.size())
    {
        _currentLevelIndex++;
        return true;
    }
    return false;
}

bool LevelManager::hasNextLevel() const {
    return (_currentLevelIndex + 1) < _levels.size();
}

void LevelManager::reset() {
    _currentLevelIndex = 0;
}