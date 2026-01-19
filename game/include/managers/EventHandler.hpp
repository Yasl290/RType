#pragma once

#include <SFML/Window/Event.hpp>
#include <engine/graphics/Renderer.hpp>
#include <engine/audio/AudioManager.hpp>
#include "core/InputState.hpp"
#include "ui/PauseMenu.hpp"
#include "ui/GameOverScreen.hpp"

enum class EventResult {
    CONTINUE,
    QUIT_APP,
    RETURN_TO_MENU,
    RESTART_GAME,
    TOGGLE_PAUSE,
    TOGGLE_DOUBLE_DAMAGE,
    TOGGLE_INVINCIBILITY,
    TOGGLE_DOUBLE_FIRE_RATE
};

class EventHandler {
public:
    EventHandler(Renderer& renderer, AudioManager& audio);
    
    EventResult handleEvent(
        const sf::Event& event,
        InputState& inputState,
        bool& paused,
        bool& gameOverShown,
        PauseMenu& pauseMenu,
        GameOverScreen& gameOverScreen
    );
    
    void updateViews();

private:
    EventResult handleWindowEvents(const sf::Event& event);
    EventResult handleGameOverEvents(const sf::Event& event, GameOverScreen& gameOverScreen);
    EventResult handlePauseEvents(const sf::Event& event, PauseMenu& pauseMenu, bool& paused);
    EventResult handleGameplayEvents(const sf::Event& event, InputState& inputState);
    
    Renderer& _renderer;
    AudioManager& _audio;
};