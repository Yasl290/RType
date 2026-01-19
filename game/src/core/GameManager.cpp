#include "core/GameManager.hpp"
#include "core/MenuLoop.hpp"
#include "core/GameLoop.hpp"
#include "initialization/GameInitializer.hpp"
#include "config/GameConstants.hpp"
#include "config/ServerConfig.hpp"
#include "NetworkClient.hpp"
#include "NetworkScoreProvider.hpp"
#include "LocalScoreProvider.hpp"
#include "ui/LobbyScreen.hpp"
#include "levels/LevelAssetManager.hpp"
#include "levels/ILevelRenderer.hpp" 
#include <engine/core/Registry.hpp>
#include <engine/core/System.hpp>
#include <engine/graphics/Sprite.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

GameManager::GameManager()
    : _renderer(GameConstants::WINDOW_WIDTH, GameConstants::WINDOW_HEIGHT, "R-Type")
    , _audioManager()
    , _menu(GameConstants::WINDOW_WIDTH, GameConstants::WINDOW_HEIGHT, _audioManager)
{
    _audioManager.playMusic("assets/audio/menu_ost.mp3");
    printWelcomeMessage();
}

GameManager::~GameManager() {
    Sprite::clearTextureCache();
}

void GameManager::run() {
    while (true) {
        int menuChoice = runMenuLoop();
        
        if (menuChoice == 0) {
            break;
        }
        
        bool isSoloMode = (menuChoice == 2);
        runGameSession(isSoloMode);
        
        _audioManager.playMusic("assets/audio/menu_ost.mp3");
    }
}

int GameManager::runMenuLoop() {
    MenuLoop menuLoop(_renderer, _menu);
    return menuLoop.run();
}

void GameManager::runGameSession(bool isSoloMode) {
    bool shouldReturnToMenu = false;
    
    while (!shouldReturnToMenu) {
        NetworkClient network;
        
        if (isSoloMode) {
            network.setSoloMode(true);
            std::cout << "[SOLO MODE] Starting endless mode..." << std::endl;
        } else {
            std::cout << "Connecting to " << Config::Server::IP << ":" 
                      << Config::Server::PORT << "..." << std::endl;
                      
            if (!network.connect(Config::Server::IP, 
                                Config::Server::PORT, 
                                Config::Server::PLAYER_NAME)) {
                std::cerr << "Failed to connect to server!" << std::endl;
                shouldReturnToMenu = true;
                break;
            }
            
            std::cout << "Connected! Client ID: " << network.getClientId() << std::endl;
        }

        if (!isSoloMode) {
            LobbyScreen lobby(network, _audioManager);
            LobbyScreenResult lobbyResult = lobby.run(_renderer);
            
            if (lobbyResult != LobbyScreenResult::START_GAME) {
                network.disconnect();
                if (lobbyResult == LobbyScreenResult::QUIT_APP) {
                    _renderer.close();
                    return;
                }
                shouldReturnToMenu = true;
                break;
            }
        }

        sf::Vector2u currentSize = _renderer.getWindow().getSize();
        float gameWidth = static_cast<float>(currentSize.x);
        float gameHeight = static_cast<float>(currentSize.y);

        Registry registry;
        std::vector<std::unique_ptr<System>> systems;
        LevelAssetManager levelAssets;

        if (isSoloMode) {
            LocalScoreProvider scoreProvider(registry);
            GameInitializer::initSystems(systems, gameWidth, gameHeight, scoreProvider, isSoloMode);
            const std::string& backgroundPath = levelAssets.getCurrentLevel()->getBackgroundPath();
            GameInitializer::initBackground(registry, gameWidth, gameHeight, backgroundPath);
            GameInitializer::initSoloPlayer(registry);

            std::cout << "Controls: Arrows/ZQSD, Space to shoot" << std::endl;
            _audioManager.playMusic("assets/audio/game_stage1_ost.mp3");

            GameLoop gameLoop(_renderer, registry, systems, network, _audioManager, isSoloMode);
            gameLoop.setLevelRenderer(levelAssets.getCurrentLevel());
            gameLoop.setLevelAssetManager(&levelAssets);
            
            GameLoopResult result = gameLoop.run();

            switch (result) {
                case GameLoopResult::QUIT_APP:
                    _renderer.close();
                    return;
                    
                case GameLoopResult::RESTART_GAME:
                    std::cout << "Restarting game..." << std::endl;
                    levelAssets.reset();
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    break;
                    
                case GameLoopResult::RETURN_TO_MENU:
                    shouldReturnToMenu = true;
                    break;
            }
        } else {
            NetworkScoreProvider scoreProvider(network);
            GameInitializer::initSystems(systems, gameWidth, gameHeight, scoreProvider, isSoloMode);
            const std::string& backgroundPath = levelAssets.getCurrentLevel()->getBackgroundPath();
            GameInitializer::initBackground(registry, gameWidth, gameHeight, backgroundPath);

            std::cout << "Controls: Arrows/ZQSD, Space to shoot" << std::endl;
            _audioManager.playMusic("assets/audio/game_stage1_ost.mp3");

            GameLoop gameLoop(_renderer, registry, systems, network, _audioManager, isSoloMode);
            gameLoop.setLevelRenderer(levelAssets.getCurrentLevel());
            gameLoop.setLevelAssetManager(&levelAssets);
            
            GameLoopResult result = gameLoop.run();

            network.disconnect();

            switch (result) {
                case GameLoopResult::QUIT_APP:
                    _renderer.close();
                    return;
                    
                case GameLoopResult::RESTART_GAME:
                    std::cout << "Restarting game..." << std::endl;
                    levelAssets.reset();
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    break;
                    
                case GameLoopResult::RETURN_TO_MENU:
                    shouldReturnToMenu = true;
                    break;
            }
        }
    }
}

void GameManager::printWelcomeMessage() {
    std::cout << "=== R-TYPE CLIENT ===" << std::endl;
    std::cout << "Press F11 to toggle fullscreen" << std::endl;
    std::cout << "Server: " << Config::Server::IP << ":" 
              << Config::Server::PORT << std::endl;
}