#pragma once

#include <engine/graphics/Renderer.hpp>
#include <engine/audio/AudioManager.hpp>
#include "Menu.hpp"

class GameManager {
public:
    GameManager();
    ~GameManager();
    
    void run();

private:
    int runMenuLoop();
    void runGameSession(bool isSoloMode);
    void printWelcomeMessage();
    
    Renderer _renderer;
    AudioManager _audioManager;
    Menu _menu;
};