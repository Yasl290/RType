#include "managers/EventHandler.hpp"
#include "config/GameConstants.hpp"

EventHandler::EventHandler(Renderer &renderer, AudioManager &audio)
    : _renderer(renderer), _audio(audio) {}

void EventHandler::updateViews() {
  sf::Vector2u size = _renderer.getWindow().getSize();
  sf::View view(sf::FloatRect(0.f, 0.f, static_cast<float>(size.x),
                              static_cast<float>(size.y)));
  _renderer.getWindow().setView(view);
}

EventResult EventHandler::handleEvent(const sf::Event &event,
                                      InputState &inputState, bool &paused,
                                      bool &gameOverShown, PauseMenu &pauseMenu,
                                      GameOverScreen &gameOverScreen) {
  EventResult result = handleWindowEvents(event);
  if (result != EventResult::CONTINUE) {
    return result;
  }

  if (gameOverShown) {
    return handleGameOverEvents(event, gameOverScreen);
  }

  if (event.type == sf::Event::KeyPressed &&
      event.key.code == sf::Keyboard::Escape) {
    paused = !paused;
    inputState.reset();
    return EventResult::TOGGLE_PAUSE;
  }

  if (paused) {
    return handlePauseEvents(event, pauseMenu, paused);
  }

  return handleGameplayEvents(event, inputState);
}

EventResult EventHandler::handleWindowEvents(const sf::Event &event) {
  if (event.type == sf::Event::Closed) {
    _renderer.close();
    return EventResult::QUIT_APP;
  }

  if (event.type == sf::Event::Resized) {
    updateViews();
    return EventResult::CONTINUE;
  }

  if (event.type == sf::Event::KeyPressed) {
    if (event.key.code == sf::Keyboard::F11) {
      _renderer.toggleFullscreen();
      updateViews();
      return EventResult::CONTINUE;
    }

    if (event.key.code == sf::Keyboard::M) {
      _audio.toggleMusicEnabled();
      return EventResult::CONTINUE;
    }
  }

  return EventResult::CONTINUE;
}

EventResult EventHandler::handleGameOverEvents(const sf::Event &event,
                                               GameOverScreen &gameOverScreen) {
  GameOverAction action = gameOverScreen.handleEvent(event);

  switch (action) {
  case GameOverAction::RESTART:
    return EventResult::RESTART_GAME;

  case GameOverAction::RETURN_TO_MENU:
    return EventResult::RETURN_TO_MENU;

  case GameOverAction::QUIT:
    _renderer.close();
    return EventResult::QUIT_APP;

  case GameOverAction::NONE:
  default:
    return EventResult::CONTINUE;
  }
}

EventResult EventHandler::handlePauseEvents(const sf::Event &event,
                                            PauseMenu &pauseMenu,
                                            bool &paused) {
  PauseMenuAction action = pauseMenu.handleEvent(event);

  switch (action) {
  case PauseMenuAction::RESUME:
    paused = false;
    return EventResult::CONTINUE;

  case PauseMenuAction::RETURN_TO_MENU:
    return EventResult::RETURN_TO_MENU;

  case PauseMenuAction::QUIT_APP:
    _renderer.close();
    return EventResult::QUIT_APP;

  case PauseMenuAction::TOGGLE_DOUBLE_DAMAGE:
    return EventResult::TOGGLE_DOUBLE_DAMAGE;

  case PauseMenuAction::TOGGLE_INVINCIBILITY:
    return EventResult::TOGGLE_INVINCIBILITY;

  case PauseMenuAction::TOGGLE_DOUBLE_FIRE_RATE:
    return EventResult::TOGGLE_DOUBLE_FIRE_RATE;

  case PauseMenuAction::NONE:
  default:
    return EventResult::CONTINUE;
  }
}

EventResult EventHandler::handleGameplayEvents(const sf::Event &event,
                                               InputState &inputState) {
  if (event.type == sf::Event::KeyPressed) {
    inputState.setKey(event.key.code, true);
  } else if (event.type == sf::Event::KeyReleased) {
    inputState.setKey(event.key.code, false);
  } else if (event.type == sf::Event::LostFocus) {
    inputState.reset();
  }

  return EventResult::CONTINUE;
}