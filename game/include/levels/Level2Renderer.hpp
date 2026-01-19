#pragma once
#include "ILevelRenderer.hpp"
#include "engine/graphics/SpriteSheet.hpp"
#include "engine/gameplay/Enemy.hpp"
#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

class Level2Renderer : public ILevelRenderer {
private:
    mutable std::unique_ptr<SpriteSheet> _enemyBasicSpriteSheet;
    mutable std::unique_ptr<SpriteSheet> _enemyFastShooterSpriteSheet;
    mutable std::unique_ptr<SpriteSheet> _enemyBomberSpriteSheet;
    mutable std::vector<std::string> _bossFrames;
    mutable std::vector<std::string> _bossShotFrames;
    mutable bool _initialized;
    
    std::vector<std::string> loadFramesFromDirectory(const char* directory) const {
        std::vector<std::string> frames;
        
        if (!std::filesystem::exists(directory)) {
            std::cerr << "[Level2Renderer] Directory does not exist: " << directory << std::endl;
            return frames;
        }
        
        std::vector<std::string> filenames;
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".png") {
                filenames.push_back(entry.path().filename().string());
            }
        }
        
        std::sort(filenames.begin(), filenames.end());
        
        for (const auto& filename : filenames) {
            frames.push_back(std::string(directory) + "/" + filename);
        }
        
        return frames;
    }
    
    void loadAssets() const {
        if (_initialized) return;
        
        _enemyBasicSpriteSheet = std::make_unique<SpriteSheet>();
        if (!_enemyBasicSpriteSheet->loadFromFile("assets/sprites/r-typesheet42.png", 376, 484, 5, 6)) {
            std::cerr << "[Level2Renderer] Failed to load basic enemy sprite sheet!" << std::endl;
        }
        
        _enemyFastShooterSpriteSheet = std::make_unique<SpriteSheet>();
        if (!_enemyFastShooterSpriteSheet->loadFromFile("assets/sprites/enemy_fastshooter_sheet.png", 456, 407, 5, 6)) {
            std::cerr << "[Level2Renderer] Failed to load fast shooter sprite sheet, using basic" << std::endl;
            _enemyFastShooterSpriteSheet = std::make_unique<SpriteSheet>();
            _enemyFastShooterSpriteSheet->loadFromFile("assets/sprites/r-typesheet42.png", 376, 484, 5, 6);
        }
        
        _enemyBomberSpriteSheet = std::make_unique<SpriteSheet>();
        if (!_enemyBomberSpriteSheet->loadFromFile("assets/sprites/enemy_bomber_sheet.png", 506, 472, 5, 6)) {
            std::cerr << "[Level2Renderer] Failed to load bomber sprite sheet, using basic" << std::endl;
            _enemyBomberSpriteSheet = std::make_unique<SpriteSheet>();
            _enemyBomberSpriteSheet->loadFromFile("assets/sprites/r-typesheet42.png", 376, 484, 5, 6);
        }
        
        _bossFrames = loadFramesFromDirectory("assets/sprites/boss_sprite");
        _bossShotFrames = loadFramesFromDirectory("assets/sprites/boss_shoot");
        
        _initialized = true;
    }
    
public:
    Level2Renderer() : _initialized(false) {}
    
    const std::vector<std::string>& getEnemyBasicFrames() const override {
        loadAssets();
        static std::vector<std::string> empty;
        return empty; 
    }
    
    const std::vector<std::string>& getBossFrames() const override {
        loadAssets();
        return _bossFrames;
    }
    
    const std::vector<std::string>& getBossShotFrames() const override {
        loadAssets();
        return _bossShotFrames;
    }
    
    const std::string& getBackgroundPath() const override {
        static const std::string path = "assets/sprites/level2/background/backgroundlvl2.jpg";
        return path;
    }
    
    const std::string& getMusicPath() const override {
        static const std::string path = "assets/audio/game_stage1_ost.mp3";
        return path;
    }
    
    const char* getName() const override {
        return "Level 2: Imperial Assault";
    }

    const SpriteSheet* getEnemySpriteSheet(EnemyType type = EnemyType::Basic) const {
        loadAssets();
        switch (type) {
            case EnemyType::FastShooter:
                return _enemyFastShooterSpriteSheet.get();
            case EnemyType::Bomber:
                return _enemyBomberSpriteSheet.get();
            case EnemyType::Basic:
            default:
                return _enemyBasicSpriteSheet.get();
        }
    }

    const SpriteSheet* getEnemySpriteSheet() const {
        return getEnemySpriteSheet(EnemyType::Basic);
    }

    int getEnemyStartFrame() const { return 10; }
    int getEnemyFrameCount() const { return 5; }
    
    int getEnemySpriteOffsetY(EnemyType type = EnemyType::Basic) const {
        switch (type) {
            case EnemyType::Bomber:
                return 12;
            case EnemyType::FastShooter:
                return 12;
            case EnemyType::Basic:
            default:
                return 12;
        }
    }
    
    float getEnemyScale(EnemyType type = EnemyType::Basic) const {
        return 1.1f;
    }
    
    float getEnemyWidth(EnemyType type = EnemyType::Basic) const {
        switch (type) {
            case EnemyType::Bomber:
                return 130.f;
            case EnemyType::FastShooter:
                return 100.f;
            case EnemyType::Basic:
            default:
                return 110.f;
        }
    }
    
    float getEnemyHeight(EnemyType type = EnemyType::Basic) const {
        switch (type) {
            case EnemyType::Bomber:
                return 85.f;
            case EnemyType::FastShooter:
                return 65.f;
            case EnemyType::Basic:
            default:
                return 75.f;
        }
    }
};