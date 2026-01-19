#pragma once
#include <engine/graphics/Renderer.hpp>
#include <engine/core/Registry.hpp>
#include <engine/core/System.hpp>
#include <engine/audio/AudioManager.hpp>
#include <engine/gameplay/UpgradeSelector.hpp>
#include "NetworkClient.hpp"
#include <vector>
#include <memory>

enum class GameLoopResult {
    RETURN_TO_MENU,
    RESTART_GAME,
    QUIT_APP
};

class GameLoop {
public:
    GameLoop(
        Renderer& renderer,
        Registry& registry,
        std::vector<std::unique_ptr<System>>& systems,
        NetworkClient& network,
        AudioManager& audioManager,
        bool isSoloMode = false
    );
    
    void setLevelRenderer(class ILevelRenderer* levelRenderer);
    void setLevelAssetManager(class LevelAssetManager* levelAssets);
    
    GameLoopResult run();

private:
    void processGameLogic(float dt);
    void checkGameOver();
    void checkLevelTransition();
    void render();
    void checkUpgradeThreshold();
    void showUpgradeMenu();
    
    Renderer& _renderer;
    Registry& _registry;
    std::vector<std::unique_ptr<System>>& _systems;
    NetworkClient& _network;
    AudioManager& _audio;
    
    class NetworkEntityManager* _entityManager;
    class EventHandler* _eventHandler;
    class InputState* _inputState;
    class PauseMenu* _pauseMenu;
    class GameOverScreen* _gameOverScreen;
    class CheatManager* _cheatManager;
    class LevelTransition* _levelTransition;
    class LevelAssetManager* _levelAssets;
    class UpgradeMenu* _upgradeMenu;
    class EndlessGameOverScreen* _endlessGameOverScreen;
    class HighscoreSystem* _highscoreSystem;
    
    bool _paused;
    bool _gameOverShown;
    bool _levelTransitionShown;
    bool _isSoloMode;
    bool _upgradeMenuShown;
    bool _endlessGameOverShown;
    
    UpgradeSelector _upgradeSelector;
};