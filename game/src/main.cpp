#include <engine/graphics/Renderer.hpp>
#include <engine/core/Registry.hpp>
#include <engine/core/System.hpp>
#include <engine/graphics/Sprite.hpp>
#include <engine/physics/Transform.hpp>
#include <engine/physics/Velocity.hpp>
#include <engine/gameplay/Controllable.hpp>
#include <engine/gameplay/Background.hpp>
#include <engine/systems/RenderSystem.hpp>
#include <engine/systems/MovementSystem.hpp>
#include <engine/systems/BoundarySystem.hpp>
#include <engine/systems/ScrollingBackgroundSystem.hpp>
#include <engine/systems/ShootingSystem.hpp>
#include <engine/systems/CleanupSystem.hpp>
#include <engine/audio/AudioManager.hpp>
#include <engine/systems/EnemyHealthBarSystem.hpp>
#include <engine/systems/ScoreDisplaySystem.hpp>
#include "NetworkScoreProvider.hpp"
#include <engine/gameplay/Health.hpp>
#include <engine/gameplay/Enemy.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include "Menu.hpp"
#include "../include/NetworkClient.hpp"
#include "ui/LobbyScreen.hpp"
#include "ui/PauseMenu.hpp"
#include "ui/GameOverScreen.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>

struct InputState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool shoot = false;

    void setKey(sf::Keyboard::Key key, bool pressed)
    {
        switch (key) {
            case sf::Keyboard::Up:
            case sf::Keyboard::Z:
                up = pressed;
                break;
            case sf::Keyboard::Down:
            case sf::Keyboard::S:
                down = pressed;
                break;
            case sf::Keyboard::Left:
            case sf::Keyboard::Q:
                left = pressed;
                break;
            case sf::Keyboard::Right:
            case sf::Keyboard::D:
                right = pressed;
                break;
            case sf::Keyboard::Space:
                shoot = pressed;
                break;
            default:
                break;
        }
    }

    void reset()
    {
        up = down = left = right = shoot = false;
    }

    uint8_t toFlags() const
    {
        uint8_t flags = 0;
        if (up) {
            flags |= 0x01;
        }
        if (down) {
            flags |= 0x02;
        }
        if (left) {
            flags |= 0x04;
        }
        if (right) {
            flags |= 0x08;
        }
        if (shoot) {
            flags |= 0x10;
        }
        return flags;
    }
};

enum class GameState {
    MENU,
    PLAYING
};

enum class GameLoopResult {
    RETURN_TO_MENU,
    RESTART_GAME,
    QUIT_APP
};

static void updateMenuView(sf::RenderWindow& window)

{
    sf::Vector2u size = window.getSize();
    sf::View view(sf::FloatRect(0.f, 0.f,
                                static_cast<float>(size.x),
                                static_cast<float>(size.y)));
    window.setView(view);
}

static void updateGameView(sf::RenderWindow& window)
{
    constexpr float LOGICAL_WIDTH = 1280.f;
    constexpr float LOGICAL_HEIGHT = 720.f;

    sf::View view(sf::FloatRect(0.f, 0.f, LOGICAL_WIDTH, LOGICAL_HEIGHT));

    sf::Vector2u size = window.getSize();
    float windowRatio = static_cast<float>(size.x) / static_cast<float>(size.y);
    float viewRatio = LOGICAL_WIDTH / LOGICAL_HEIGHT;

    float viewportX = 0.f;
    float viewportY = 0.f;
    float viewportW = 1.f;
    float viewportH = 1.f;

    if (windowRatio > viewRatio) {
        viewportW = viewRatio / windowRatio;
        viewportX = (1.f - viewportW) * 0.5f;
    } else if (windowRatio < viewRatio) {
        viewportH = windowRatio / viewRatio;
        viewportY = (1.f - viewportH) * 0.5f;
    }

    view.setViewport(sf::FloatRect(viewportX, viewportY, viewportW, viewportH));
    window.setView(view);
}

void initSystems(std::vector<std::unique_ptr<System>>& systems, float width, float /*height*/, IScoreProvider& scoreProvider)
{
    systems.push_back(std::make_unique<MovementSystem>());
    systems.push_back(std::make_unique<ScrollingBackgroundSystem>(width));
    systems.push_back(std::make_unique<RenderSystem>());
    systems.push_back(std::make_unique<EnemyHealthBarSystem>());
    systems.push_back(std::make_unique<ScoreDisplaySystem>(scoreProvider));
}

void initBackground(Registry& registry, float windowWidth, float windowHeight)
{
    const float bgNativeWidth = 1280.f;
    const float bgNativeHeight = 720.f;
    float scaleX = windowWidth / bgNativeWidth;
    float scaleY = windowHeight / bgNativeHeight;

    EntityID bg1 = registry.create();
    registry.add<Sprite>(bg1, "assets/sprites/background.png");
    Transform& t1 = registry.add<Transform>(bg1, 0.f, 0.f);
    t1.scaleX = scaleX;
    t1.scaleY = scaleY;
    registry.add<Velocity>(bg1, -100.f, 0.f);
    registry.add<Background>(bg1);

    EntityID bg2 = registry.create();
    registry.add<Sprite>(bg2, "assets/sprites/background.png");
    Transform& t2 = registry.add<Transform>(bg2, windowWidth, 0.f);
    t2.scaleX = scaleX;
    t2.scaleY = scaleY;
    registry.add<Velocity>(bg2, -100.f, 0.f);
    registry.add<Background>(bg2);
}

void initPlayer(Registry& registry, float windowHeight)
{
    EntityID player = registry.create();

    Sprite& sprite = registry.add<Sprite>(player, "assets/sprites/player.png");
    sprite.getSprite().setTextureRect(sf::IntRect(0, 30, 316, 160));

Transform& transform = registry.add<Transform>(player, 100.f, windowHeight / 2.f - 80.f);
    transform.scaleX = 0.4f;
    transform.scaleY = 0.4f;

    registry.add<Velocity>(player, 0.f, 0.f);
    registry.add<Controllable>(player, 250.f);
    registry.add<Health>(player, 100.f);
}

std::string getPlayerTexture(uint8_t slot)
{
    switch (slot) {
        case 0:
            return "assets/sprites/player.png";
        case 1:
            return "assets/sprites/player_pink.png";
        case 2:
            return "assets/sprites/player_green.png";
        case 3:
            return "assets/sprites/player_yellow.png";
        default:
            return "assets/sprites/player.png";
    }
}

GameLoopResult gameLoop(Renderer& renderer,
    Registry& registry,
    std::vector<std::unique_ptr<System>>& systems,
    NetworkClient& network,
    AudioManager& audioManager)
{
    sf::Clock clock;
    updateGameView(renderer.getWindow());

    const float DESPAWN_TIMEOUT = 0.1f;

    struct NetEntityInfo {
        EntityID id;
        float lastSeen;
    };
    std::unordered_map<uint32_t, NetEntityInfo> networkEntities;

    InputState inputState;

    bool paused = false;
    bool gameOverShown = false;

    PauseMenu pauseMenu(audioManager);
    pauseMenu.onResize(renderer.getWindow().getSize());
    
    GameOverScreen gameOverScreen;
    gameOverScreen.onResize(renderer.getWindow().getSize());

    while (renderer.isOpen()) {
        float dt = clock.restart().asSeconds();

        sf::Event event;
        while (renderer.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                renderer.close();
                return GameLoopResult::QUIT_APP;
            }

            if (event.type == sf::Event::Resized) {
                updateGameView(renderer.getWindow());
                pauseMenu.onResize(renderer.getWindow().getSize());
                gameOverScreen.onResize(renderer.getWindow().getSize());
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F11) {
                renderer.toggleFullscreen();
                updateGameView(renderer.getWindow());
                pauseMenu.onResize(renderer.getWindow().getSize());
                gameOverScreen.onResize(renderer.getWindow().getSize());
                continue;
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::M) {
                audioManager.toggleMusicEnabled();
                continue;
            }

            if (gameOverShown) {
                GameOverAction action = gameOverScreen.handleEvent(event);
                switch (action) {
                    case GameOverAction::RESTART:
                        network.resetGameOver();
                        
                        for (auto& [net_id, info] : networkEntities) {
                            registry.markForDestruction(info.id);
                        }
                        networkEntities.clear();
                        registry.cleanup();
                        
                        return GameLoopResult::RESTART_GAME;
                    case GameOverAction::RETURN_TO_MENU:
                        for (auto& [net_id, info] : networkEntities) {
                            registry.markForDestruction(info.id);
                        }
                        networkEntities.clear();
                        registry.cleanup();
                        return GameLoopResult::RETURN_TO_MENU;
                    case GameOverAction::QUIT:
                        renderer.close();
                        return GameLoopResult::QUIT_APP;
                    case GameOverAction::NONE:
                        break;
                }
                continue;
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                paused = !paused;
                inputState.reset();
                continue;
            }

            if (!paused) {
                if (event.type == sf::Event::KeyPressed) {
                    inputState.setKey(event.key.code, true);
                } else if (event.type == sf::Event::KeyReleased) {
                    inputState.setKey(event.key.code, false);
                } else if (event.type == sf::Event::LostFocus) {
                    inputState.reset();
                }
            } else {
                PauseMenuAction action = pauseMenu.handleEvent(event);
                switch (action) {
                    case PauseMenuAction::RESUME:
                        paused = false;
                        inputState.reset();
                        break;
                    case PauseMenuAction::RETURN_TO_MENU:
                        for (auto& [net_id, info] : networkEntities) {
                            registry.markForDestruction(info.id);
                        }
                        networkEntities.clear();
                        registry.cleanup();
                        return GameLoopResult::RETURN_TO_MENU;
                    case PauseMenuAction::QUIT_APP:
                        renderer.close();
                        return GameLoopResult::QUIT_APP;
                    case PauseMenuAction::NONE:
                        break;
                }
            }
        }
        
        if (network.isGameOver() && !gameOverShown) {
            gameOverShown = true;
            paused = true;
            inputState.reset();
            
            auto finalScores = network.getFinalScores();
            gameOverScreen.setScores(finalScores);
        }

        if (network.isConnected() && !gameOverShown) {
            uint8_t flags = paused ? 0 : inputState.toFlags();
            network.sendInput(flags);
        }

        auto updates = network.pollEntityUpdates();
        for (const auto& update : updates) {
            EntityID local_entity;
            auto it = networkEntities.find(update.entity_id);
            
            if (it == networkEntities.end()) {
                local_entity = registry.create();
                networkEntities[update.entity_id] = { local_entity, 0.f };
                
                registry.add<Transform>(local_entity, update.pos_x, update.pos_y);
                if (update.entity_type == 0) {
                    std::string texturePath = getPlayerTexture(update.player_slot);
                    Sprite& sprite = registry.add<Sprite>(local_entity, texturePath);
                    sprite.getSprite().setTextureRect(sf::IntRect(0, 30, 316, 160));
                    Transform& t = registry.get<Transform>(local_entity);
                    t.scaleX = 0.4f;
                    t.scaleY = 0.4f;
                    Health& h = registry.has<Health>(local_entity)
                        ? registry.get<Health>(local_entity)
                        : registry.add<Health>(local_entity, update.hp_max > 0.f ? update.hp_max : 100.f);
                    h.max = update.hp_max > 0.f ? update.hp_max : h.max;
                    h.current = update.hp_current;
                } else if (update.entity_type == 1) {
                    registry.add<Sprite>(local_entity, "assets/sprites/enemy_basic.png");
                    Transform& t = registry.get<Transform>(local_entity);
                    t.scaleX = 0.6f;
                    t.scaleY = 0.6f;
                    if (!registry.has<Enemy>(local_entity)) {
                        registry.add<Enemy>(local_entity, EnemyType::Basic);
                    }
                    Health& h = registry.has<Health>(local_entity)
                        ? registry.get<Health>(local_entity)
                        : registry.add<Health>(local_entity, update.hp_max > 0.f ? update.hp_max : 50.f);
                    h.max = update.hp_max > 0.f ? update.hp_max : h.max;
                    h.current = update.hp_current;
                } else if (update.entity_type == 2) {
                    registry.add<Sprite>(local_entity, "assets/sprites/bullet_normal.png");
                } else if (update.entity_type == 3) {
                    registry.add<Sprite>(local_entity, "assets/sprites/bullet_charged.png");
                } else if (update.entity_type == 4) {
                    registry.add<Sprite>(local_entity, "assets/sprites/enemy-shoot.png");
                    Transform& t = registry.get<Transform>(local_entity);
                    t.scaleX = 0.7f;
                    t.scaleY = 0.7f;
                }
            } else {
                local_entity = it->second.id;
                it->second.lastSeen = 0.f;
                
                if (registry.has<Transform>(local_entity)) {
                    Transform& transform = registry.get<Transform>(local_entity);
                    transform.x = update.pos_x;
                    transform.y = update.pos_y;
                }

                if ((update.entity_type == 0 || update.entity_type == 1) && update.hp_max > 0.f) {
                    Health& h = registry.has<Health>(local_entity)
                        ? registry.get<Health>(local_entity)
                        : registry.add<Health>(local_entity, update.hp_max);
                    h.max = update.hp_max;
                    h.current = update.hp_current;
                }
            }
        }

        for (auto it = networkEntities.begin(); it != networkEntities.end(); ) {
            it->second.lastSeen += dt;
            if (it->second.lastSeen > DESPAWN_TIMEOUT) {
                registry.markForDestruction(it->second.id);
                it = networkEntities.erase(it);
            } else {
                ++it;
            }
        }

        if (!paused && !gameOverShown) {
            for (auto& sys : systems) {
                sys->update(registry, dt);
            }
            registry.cleanup();
        }

        renderer.clear(sf::Color::Black);
        for (auto& sys : systems) {
            sys->render(registry, renderer);
        }

        if (gameOverShown) {
            gameOverScreen.render(renderer.getWindow());
        } else if (paused) {
            pauseMenu.render(renderer.getWindow());
        }

        renderer.display();
    }

    return GameLoopResult::QUIT_APP;
}

bool menuLoop(Renderer& renderer, Menu& menu)
{
    updateMenuView(renderer.getWindow());

    while (renderer.isOpen()) {
        sf::Event event;
        while (renderer.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                renderer.close();
                return false;
            }

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F11) {
                renderer.toggleFullscreen();
                updateMenuView(renderer.getWindow());
                sf::Vector2u size = renderer.getWindow().getSize();
                menu.resize(static_cast<float>(size.x), static_cast<float>(size.y));
            }

            MenuAction action = menu.handleEvent(event);
            switch (action) {
                case MenuAction::PLAY:
                    std::cout << ">> PLAY clicked - Ready to play!" << std::endl;
                    return true;
                case MenuAction::QUIT:
                    std::cout << ">> QUIT clicked" << std::endl;
                    renderer.close();
                    return false;
                case MenuAction::RES_1024x576:
                    renderer.setWindowedSize(1024, 576);
                    updateMenuView(renderer.getWindow());
                    menu.resize(1024.f, 576.f);
                    break;
                case MenuAction::RES_1280x720:
                    renderer.setWindowedSize(1280, 720);
                    updateMenuView(renderer.getWindow());
                    menu.resize(1280.f, 720.f);
                    break;
                case MenuAction::RES_1600x900:
                    renderer.setWindowedSize(1600, 900);
                    updateMenuView(renderer.getWindow());
                    menu.resize(1600.f, 900.f);
                    break;
                case MenuAction::TOGGLE_FULLSCREEN: {
                    renderer.toggleFullscreen();
                    updateMenuView(renderer.getWindow());
                    sf::Vector2u size = renderer.getWindow().getSize();
                    menu.resize(static_cast<float>(size.x), static_cast<float>(size.y));
                    break;
                }
                case MenuAction::SETTINGS:
                case MenuAction::BACK:
                case MenuAction::NONE:
                    break;
            }
        }

        renderer.clear(sf::Color(20, 20, 30));
        menu.render(renderer.getWindow());
        renderer.display();
    }
    return false;
}

int main()
{
    const float windowWidth = 1280.f;
    const float windowHeight = 720.f;

    Renderer renderer(windowWidth, windowHeight, "R-Type");
    AudioManager audioManager;
    audioManager.playMusic("assets/audio/menu_ost.mp3");

    Menu menu(windowWidth, windowHeight, audioManager);
    
    std::cout << "=== R-TYPE CLIENT ===" << std::endl;
    std::cout << "Press F11 to toggle fullscreen" << std::endl;

    while (true) {
        if (!menuLoop(renderer, menu)) {
            break;
        }

        bool shouldReturnToMenu = false;
        
        while (!shouldReturnToMenu) {
            NetworkClient network;
            std::cout << "Connecting to server..." << std::endl;
            if (!network.connect("127.0.0.1", 4242, "Player")) {
                std::cerr << "Failed to connect to server!" << std::endl;
                shouldReturnToMenu = true;
                break;
            }
            std::cout << "Connected! Client ID: " << network.getClientId() << std::endl;

            NetworkScoreProvider scoreProvider(network); 
            LobbyScreen lobby(network, audioManager);
            LobbyScreenResult lobbyResult = lobby.run(renderer);
            if (lobbyResult != LobbyScreenResult::START_GAME) {
                network.disconnect();

                if (lobbyResult == LobbyScreenResult::QUIT_APP) {
                    Sprite::clearTextureCache();
                    return 0;
                }

                audioManager.playMusic("assets/audio/menu_ost.mp3");
                shouldReturnToMenu = true;
                break;
            }

            sf::Vector2u currentSize = renderer.getWindow().getSize();
            float gameWidth = static_cast<float>(currentSize.x);
            float gameHeight = static_cast<float>(currentSize.y);

        updateGameView(renderer.getWindow());


            Registry registry;
            std::vector<std::unique_ptr<System>> systems;

            initSystems(systems, gameWidth, gameHeight, scoreProvider);
            initBackground(registry, gameWidth, gameHeight);

            std::cout << "Controls: Arrows/ZQSD, Space to shoot" << std::endl;

            audioManager.playMusic("assets/audio/game_stage1_ost.mp3");

            GameLoopResult result = gameLoop(renderer, registry, systems, network, audioManager);

            network.disconnect();

            switch (result) {
                case GameLoopResult::QUIT_APP:
                    Sprite::clearTextureCache();
                    return 0;
                    
                case GameLoopResult::RESTART_GAME:
                    std::cout << "Restarting game..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    break;
                    
                case GameLoopResult::RETURN_TO_MENU:
                    shouldReturnToMenu = true;
                    break;
            }
        }

        audioManager.playMusic("assets/audio/menu_ost.mp3");
    }

    Sprite::clearTextureCache();
    return 0;
}