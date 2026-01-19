#include "core/GameLoop.hpp"
#include "CheatManager.hpp"
#include "core/InputState.hpp"
#include "helpers/GameClientHelpers.hpp"
#include "initialization/GameInitializer.hpp"
#include "levels/LevelAssetManager.hpp"
#include "managers/EventHandler.hpp"
#include "managers/NetworkEntityManager.hpp"
#include "ui/GameOverScreen.hpp"
#include "ui/UpgradeMenu.hpp"
#include "ui/EndlessGameOverScreen.hpp"
#include "ui/LevelTransition.hpp"
#include "ui/PauseMenu.hpp"

#include <engine/graphics/Sprite.hpp>
#include <engine/graphics/Animation.hpp>
#include <engine/gameplay/Enemy.hpp>
#include <engine/gameplay/PlayerStats.hpp>
#include <engine/gameplay/Controllable.hpp>
#include <engine/gameplay/Score.hpp>
#include <engine/gameplay/Health.hpp>
#include <engine/systems/HighscoreSystem.hpp>
#include <engine/physics/Transform.hpp>
#include <SFML/System/Clock.hpp>
#include <iostream>

GameLoop::GameLoop(
    Renderer& renderer,
    Registry& registry,
    std::vector<std::unique_ptr<System>>& systems,
    NetworkClient& network,
    AudioManager& audioManager,
    bool isSoloMode)
    : _renderer(renderer)
    , _registry(registry)
    , _systems(systems)
    , _network(network)
    , _audio(audioManager)
    , _paused(false)
    , _gameOverShown(false)
    , _levelTransitionShown(false)
    , _isSoloMode(isSoloMode)
    , _upgradeMenuShown(false)
    , _levelAssets(nullptr)
    , _endlessGameOverShown(false)
    , _highscoreSystem(nullptr)
{
    _entityManager = new NetworkEntityManager(_registry);
    _eventHandler = new EventHandler(_renderer, _audio);
    _inputState = new InputState();
    _cheatManager = new CheatManager();
    _pauseMenu = new PauseMenu(_audio, &_network, _cheatManager);
    _gameOverScreen = new GameOverScreen();
    _levelTransition = new LevelTransition();
    _upgradeMenu = new UpgradeMenu(_audio);
    _endlessGameOverScreen = new EndlessGameOverScreen();
    
    if (_isSoloMode) {
        _highscoreSystem = new HighscoreSystem();
    }
    
    _eventHandler->updateViews();
    _pauseMenu->onResize(_renderer.getWindow().getSize());
    _gameOverScreen->onResize(_renderer.getWindow().getSize());
    _levelTransition->onResize(_renderer.getWindow().getSize());
    _upgradeMenu->onResize(_renderer.getWindow().getSize());
    _endlessGameOverScreen->onResize(_renderer.getWindow().getSize());
    
    _upgradeMenu->setOnUpgradeSelected([this](UpgradeType upgradeType) {
        _registry.each<Controllable, PlayerStats>([upgradeType](EntityID, Controllable&, PlayerStats& stats) {
            stats.applyUpgrade(upgradeType);
            
            const auto* def = UpgradeDatabase::getUpgrade(upgradeType);
            if (def) {
                std::cout << "[GameLoop] Applied upgrade: " << def->name 
                          << " (Level " << stats.getUpgradeLevel(upgradeType) << ")" << std::endl;
            }
        });
        
        _upgradeMenuShown = false;
        _paused = false;
    });
}

void GameLoop::setLevelRenderer(ILevelRenderer *levelRenderer) {
  if (_entityManager) {
    _entityManager->setLevelRenderer(levelRenderer);
  }
}

void GameLoop::setLevelAssetManager(LevelAssetManager *levelAssets) {
  _levelAssets = levelAssets;
}

GameLoopResult GameLoop::run() {
    sf::Clock clock;
    
    while (_renderer.isOpen()) {
        float dt = clock.restart().asSeconds();
        
        sf::Event event;
        while (_renderer.pollEvent(event)) {
            if (_endlessGameOverShown) {
                auto action = _endlessGameOverScreen->handleEvent(event);
                
                if (action == EndlessGameOverScreen::Action::RESTART) {
                    if (_highscoreSystem) {
                        _highscoreSystem->reset();
                    }
                    delete _entityManager;
                    delete _eventHandler;
                    delete _inputState;
                    delete _pauseMenu;
                    delete _gameOverScreen;
                    delete _levelTransition;
                    delete _cheatManager;
                    delete _upgradeMenu;
                    delete _endlessGameOverScreen;
                    delete _highscoreSystem;
                    return GameLoopResult::RESTART_GAME;
                }
                
                if (action == EndlessGameOverScreen::Action::MENU) {
                    _entityManager->clearAll();
                    delete _entityManager;
                    delete _eventHandler;
                    delete _inputState;
                    delete _pauseMenu;
                    delete _gameOverScreen;
                    delete _levelTransition;
                    delete _cheatManager;
                    delete _upgradeMenu;
                    delete _endlessGameOverScreen;
                    delete _highscoreSystem;
                    return GameLoopResult::RETURN_TO_MENU;
                }
                
                continue;
            }
            
            if (_upgradeMenuShown) {
                _upgradeMenu->handleEvent(event);
                continue;
            }
            
            EventResult result = _eventHandler->handleEvent(
                event,
                *_inputState,
                _paused,
                _gameOverShown,
                *_pauseMenu,
                *_gameOverScreen
            );
            
            switch (result) {
                case EventResult::QUIT_APP:
                    delete _entityManager;
                    delete _eventHandler;
                    delete _inputState;
                    delete _pauseMenu;
                    delete _gameOverScreen;
                    delete _levelTransition;
                    delete _cheatManager;
                    delete _upgradeMenu;
                    delete _endlessGameOverScreen;
                    delete _highscoreSystem;
                    return GameLoopResult::QUIT_APP;

                case EventResult::RETURN_TO_MENU:
                    _entityManager->clearAll();
                    delete _entityManager;
                    delete _eventHandler;
                    delete _inputState;
                    delete _pauseMenu;
                    delete _gameOverScreen;
                    delete _levelTransition;
                    delete _cheatManager;
                    delete _upgradeMenu;
                    delete _endlessGameOverScreen;
                    delete _highscoreSystem;
                    return GameLoopResult::RETURN_TO_MENU;

                case EventResult::RESTART_GAME:
                    _network.resetGameOver();
                    _entityManager->clearAll();
                    delete _entityManager;
                    delete _eventHandler;
                    delete _inputState;
                    delete _pauseMenu;
                    delete _gameOverScreen;
                    delete _levelTransition;
                    delete _cheatManager;
                    delete _upgradeMenu;
                    delete _endlessGameOverScreen;
                    delete _highscoreSystem;
                    return GameLoopResult::RESTART_GAME;

                case EventResult::TOGGLE_DOUBLE_DAMAGE:
                    _cheatManager->toggleDoubleDamage();
                    _pauseMenu->onResize(_renderer.getWindow().getSize());
                    break;

                case EventResult::TOGGLE_INVINCIBILITY:
                    _cheatManager->toggleInvincibility();
                    _pauseMenu->onResize(_renderer.getWindow().getSize());
                    break;

                case EventResult::TOGGLE_DOUBLE_FIRE_RATE:
                    _cheatManager->toggleDoubleFireRate();
                    _pauseMenu->onResize(_renderer.getWindow().getSize());
                    break;

                case EventResult::TOGGLE_PAUSE:
                case EventResult::CONTINUE:
                default:
                    break;
            }
        }
        
        checkLevelTransition();
        
        if (_isSoloMode && !_paused && !_gameOverShown && !_upgradeMenuShown && !_levelTransitionShown && !_endlessGameOverShown) {
            checkUpgradeThreshold();
        }
        
        if (_levelTransitionShown) {
            _levelTransition->update(dt);
            
            if (_levelTransition->isComplete()) {
                std::cout << "[GameLoop] Transition animation complete, hiding" << std::endl;
                _levelTransition->hide();
                _levelTransitionShown = false;
            }
        }
        
        if (!_paused && !_gameOverShown && !_levelTransitionShown && !_upgradeMenuShown && !_endlessGameOverShown) {
            processGameLogic(dt);
        }
        
        checkGameOver();
        _registry.cleanup();
        
        render();
    }
    
    delete _entityManager;
    delete _eventHandler;
    delete _inputState;
    delete _pauseMenu;
    delete _gameOverScreen;
    delete _levelTransition;
    delete _cheatManager;
    delete _upgradeMenu;
    delete _endlessGameOverScreen;
    delete _highscoreSystem;
    
    return GameLoopResult::QUIT_APP;
}

void GameLoop::checkLevelTransition() {
  auto events = _network.pollGameEvents();

  for (const auto &evt : events) {
    if (evt.event_type == RType::Protocol::GameEventType::LEVEL_COMPLETE) {
      std::cout << "[GameLoop] Level transition event received: "
                << evt.levelName << " -> " << evt.nextLevelName << std::endl;

      if (_levelAssets && _levelAssets->nextLevel()) {
        std::cout << "[GameLoop] Switched to next level assets" << std::endl;

        if (_entityManager) {
          _entityManager->setLevelRenderer(_levelAssets->getCurrentLevel());
          std::cout << "[GameLoop] Updated entity manager level renderer"
                    << std::endl;
        }

        auto *currentLevel = _levelAssets->getCurrentLevel();
        if (currentLevel) {
          const std::string &musicPath = currentLevel->getMusicPath();
          std::cout << "[GameLoop] Changing music to: " << musicPath
                    << std::endl;
          _audio.playMusic(musicPath);

          const std::string &backgroundPath = currentLevel->getBackgroundPath();
          std::cout << "[GameLoop] Changing background to: " << backgroundPath
                    << std::endl;
          GameInitializer::updateBackground(_registry, backgroundPath);
        }
      }

      _levelTransition->show(evt.levelName, evt.nextLevelName);
      _levelTransitionShown = true;
      _paused = true;
      _inputState->reset();

      std::cout << "[GameLoop] Client frozen for 3 seconds transition"
                << std::endl;
    }
  }
}

void GameLoop::checkUpgradeThreshold() {
    _registry.each<Controllable, PlayerStats, Score>([this](EntityID, Controllable&, PlayerStats& stats, Score& score) {
        stats.currentScore = score.getPoints();
        
        if (stats.shouldShowUpgradeMenu()) {
            showUpgradeMenu();
        }
    });
}

void GameLoop::showUpgradeMenu() {
    _registry.each<Controllable, PlayerStats>([this](EntityID, Controllable&, PlayerStats& stats) {
        auto choices = _upgradeSelector.generateUpgradeChoices(stats, 3);
        
        if (!choices.empty()) {
            _upgradeMenu->show(choices);
            _upgradeMenuShown = true;
            _paused = true;
            _inputState->reset();
            
            std::cout << "[GameLoop] Showing upgrade menu at score " << stats.currentScore << std::endl;
        } else {
            std::cout << "[GameLoop] No upgrades available, skipping menu" << std::endl;
            stats.nextUpgradeThreshold += stats.upgradeInterval;
        }
    });
}

void GameLoop::processGameLogic(float dt) {
  static bool serverWasSuspended = false;
  static float suspendedTime = 0.0f;

  bool serverResponding = _network.isServerResponding();
  bool isConnected = _network.isConnected();

  if (isConnected && !serverResponding && !_gameOverShown) {
    if (!serverWasSuspended) {
      std::cout << "[GameLoop] Server not responding - Game frozen"
                << std::endl;
      std::cout << "[GameLoop] Waiting for server to resume (use 'fg' in "
                   "server terminal)..."
                << std::endl;
      serverWasSuspended = true;
      suspendedTime = 0.0f;
    }
    suspendedTime += dt;

    static float logTimer = 0.0f;
    logTimer += dt;
    if (logTimer >= 2.0f) {
      std::cout << "[GameLoop] Still waiting for server... (suspended for "
                << static_cast<int>(suspendedTime) << "s)" << std::endl;
      logTimer = 0.0f;
    }

    if (!_paused) {
      _paused = true;
      _inputState->reset();
    }
  } else if (isConnected && serverResponding && serverWasSuspended &&
             !_gameOverShown) {
    std::cout << "[GameLoop] Server responding again after "
              << static_cast<int>(suspendedTime) << "s - Game resumed"
              << std::endl;
    _paused = false;
    serverWasSuspended = false;
    suspendedTime = 0.0f;
  }

  if (_network.isConnected() && !_gameOverShown) {
    uint8_t flags =
        _paused ? 0 : (_inputState->toFlags() | _cheatManager->getCheatFlags());

    EntityID id = _entityManager->getLocalPlayerEntity(_network.getClientId());
    if (id != static_cast<EntityID>(-1) && _registry.has<Transform>(id)) {
      const auto &t = _registry.get<Transform>(id);
      auto size = _renderer.getWindow().getSize();

      if (t.y <= 0)
        flags &= ~0x01;
      if (t.y + 60.f >= size.y)
        flags &= ~0x02;
      if (t.x <= 0)
        flags &= ~0x04;
      if (t.x + 100.f >= size.x)
        flags &= ~0x08;
    }

    _network.sendInput(flags);
  }

  _entityManager->update(dt, _network);

  auto fireEvents = _network.pollFireEvents();
  for (const auto &event : fireEvents) {
    _audio.playSound("assets/audio/shot_sound.mp3");
  }

  {
    const auto &frames = getEnemyBasicFrames();
    if (!frames.empty()) {
      _registry.each<Enemy, Animation, Sprite>(
          [dt, &frames](EntityID, Enemy &enemy, Animation &anim,
                        Sprite &sprite) {
            if (enemy.type != EnemyType::Basic)
              return;

            anim.elapsedTime += dt;
            if (anim.elapsedTime >= anim.frameTime) {
              anim.elapsedTime = 0.f;

              if (anim.frames.size() > 0) {
                anim.currentFrame = (anim.currentFrame + 1) % anim.frameCount;
                const AnimationFrame *frame = anim.getCurrentFrame();
                if (frame) {
                  sprite.getSprite().setTextureRect(sf::IntRect(
                      frame->x, frame->y, frame->width, frame->height));
                }
              } else {
                int frameCount = static_cast<int>(frames.size());
                if (frameCount > 0) {
                  anim.currentFrame = (anim.currentFrame + 1) % frameCount;
                  int idx = anim.currentFrame;
                  if (idx >= 0 && idx < frameCount) {
                    sprite.loadTexture(frames[idx]);
                  }
                }
              }
            }
          });
    }
  }

  {
    const auto &frames = getBossFrames();
    if (!frames.empty()) {
      _registry.each<Enemy, Animation, Sprite>(
          [dt, &frames](EntityID, Enemy &enemy, Animation &anim,
                        Sprite &sprite) {
            if (enemy.type != EnemyType::Boss)
              return;

            anim.elapsedTime += dt;
            if (anim.elapsedTime >= anim.frameTime) {
              anim.elapsedTime = 0.f;
              int frameCount = static_cast<int>(frames.size());
              if (frameCount <= 0)
                return;
              anim.currentFrame = (anim.currentFrame + 1) % frameCount;

              int idx = anim.currentFrame;
              if (idx >= 0 && idx < frameCount) {
                sprite.loadTexture(frames[idx]);
              }
            }
          });
    }
  }

  {
    const auto &frames = getBossShotFrames();
    if (!frames.empty()) {
      _registry.each<Animation, Sprite>(
          [dt, &frames, this](EntityID id, Animation &anim, Sprite &sprite) {
            if (_registry.has<Enemy>(id))
              return;

            anim.elapsedTime += dt;
            if (anim.elapsedTime >= anim.frameTime) {
              anim.elapsedTime = 0.f;
              int frameCount = static_cast<int>(frames.size());
              if (frameCount <= 0)
                return;
              anim.currentFrame = (anim.currentFrame + 1) % frameCount;

              int idx = anim.currentFrame;
              if (idx >= 0 && idx < frameCount) {
                sprite.loadTexture(frames[idx]);
              }
            }
          });
    }
  }

  for (auto &sys : _systems) {
    sys->update(_registry, dt);
  }
}

void GameLoop::checkGameOver() {
    if (_network.isGameOver() && !_gameOverShown) {
        _gameOverShown = true;
        _paused = true;
        _inputState->reset();
        
        auto finalScores = _network.getFinalScores();
        _gameOverScreen->setScores(finalScores);
    }
    
    if (_isSoloMode && _highscoreSystem && !_endlessGameOverShown && !_gameOverShown) {
        if (_highscoreSystem->checkIfPlayerDead(_registry)) {
            
            uint32_t finalScore = 0;
            uint32_t finalKills = 0;
            
            bool scoreFound = false;
            _registry.each<Controllable, Score>([&](EntityID, Controllable&, Score& score) {
                finalScore = score.getPoints();
                finalKills = score.getEnemiesKilled();
                scoreFound = true;
            });
            
            if (!scoreFound) {
                std::cout << "[GameLoop] WARNING: Could not find player score!" << std::endl;
            }
            
            _endlessGameOverShown = true;
            _paused = true;
            _inputState->reset();
            
            _highscoreSystem->saveCurrentScore(_registry);
            
            const auto& highscoreData = _highscoreSystem->getHighscoreData();
            bool isNewRecord = _highscoreSystem->isNewScoreRecord(finalScore) || 
                              _highscoreSystem->isNewKillsRecord(finalKills);
            
            _endlessGameOverScreen->show(
                finalScore, 
                finalKills,
                highscoreData.bestScore,
                highscoreData.bestKills,
                isNewRecord
            );
        }
    }
}

void GameLoop::render() {
  _renderer.clear(sf::Color::Black);

  for (auto &sys : _systems) {
    sys->render(_registry, _renderer);
  }

  if (_endlessGameOverShown) {
    _endlessGameOverScreen->render(_renderer.getWindow());
  } else if (_gameOverShown) {
    _gameOverScreen->render(_renderer.getWindow());
  } else if (_upgradeMenuShown) {
    _upgradeMenu->render(_renderer.getWindow());
  } else if (_levelTransitionShown) {
    _levelTransition->render(_renderer.getWindow());
  } else if (_paused) {
    _pauseMenu->render(_renderer.getWindow());
  }

  _renderer.display();
}