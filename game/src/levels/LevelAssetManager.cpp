#include "levels/LevelAssetManager.hpp"
#include "levels/Level1Renderer.hpp"
#include "levels/Level2Renderer.hpp"

LevelAssetManager::LevelAssetManager()
    : _currentLevelIndex(0)
{
    addLevel(std::make_unique<Level1Renderer>());
    addLevel(std::make_unique<Level2Renderer>());
}

void LevelAssetManager::addLevel(std::unique_ptr<ILevelRenderer> level)
{
    _levels.push_back(std::move(level));
}

ILevelRenderer* LevelAssetManager::getCurrentLevel()
{
    if (_currentLevelIndex < _levels.size()) {
        return _levels[_currentLevelIndex].get();
    }
    return nullptr;
}

const ILevelRenderer* LevelAssetManager::getCurrentLevel() const
{
    if (_currentLevelIndex < _levels.size()) {
        return _levels[_currentLevelIndex].get();
    }
    return nullptr;
}

bool LevelAssetManager::nextLevel()
{
    if (_currentLevelIndex + 1 < _levels.size()) {
        _currentLevelIndex++;
        return true;
    }
    return false;
}

void LevelAssetManager::reset()
{
    _currentLevelIndex = 0;
}