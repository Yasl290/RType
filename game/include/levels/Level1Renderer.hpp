#pragma once
#include "ILevelRenderer.hpp"
#include <filesystem>
#include <algorithm>
#include <iostream>

class Level1Renderer : public ILevelRenderer {
private:
    mutable std::vector<std::string> _enemyFrames;
    mutable std::vector<std::string> _bossFrames;
    mutable std::vector<std::string> _bossShotFrames;
    mutable bool _initialized;
    
    void loadAssets() const {
        if (_initialized) return;
        
        namespace fs = std::filesystem;
        
        try {
            for (const auto& entry : fs::directory_iterator("assets/sprites/basic_ennemie")) {
                if (entry.path().extension() == ".png") {
                    _enemyFrames.push_back(entry.path().string());
                }
            }
            std::sort(_enemyFrames.begin(), _enemyFrames.end());
        } catch (const std::exception& e) {
            std::cerr << "[Level1Renderer] Error loading enemy frames: " << e.what() << std::endl;
        }
        try {
            for (const auto& entry : fs::directory_iterator("assets/sprites/boss_sprite")) {
                if (entry.path().extension() == ".png") {
                    _bossFrames.push_back(entry.path().string());
                }
            }
            std::sort(_bossFrames.begin(), _bossFrames.end());
        } catch (const std::exception& e) {
            std::cerr << "[Level1Renderer] Error loading boss frames: " << e.what() << std::endl;
        }
        
        try {
            for (const auto& entry : fs::directory_iterator("assets/sprites/boss_shoot")) {
                if (entry.path().extension() == ".png") {
                    _bossShotFrames.push_back(entry.path().string());
                }
            }
            std::sort(_bossShotFrames.begin(), _bossShotFrames.end());
        } catch (const std::exception& e) {
            std::cerr << "[Level1Renderer] Error loading boss shot frames: " << e.what() << std::endl;
        }
        
        _initialized = true;
    }
    
public:
    Level1Renderer() : _initialized(false) {}
    
    const std::vector<std::string>& getEnemyBasicFrames() const override {
        loadAssets();
        return _enemyFrames;
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
        static const std::string path = "assets/sprites/background.png";
        return path;
    }
    
    const std::string& getMusicPath() const override {
        static const std::string path = "assets/audio/game_stage1_ost.mp3";
        return path;
    }
    
    const char* getName() const override {
        return "Level 1: The Beginning";
    }
};