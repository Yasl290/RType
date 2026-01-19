#include "initialization/GameInitializer.hpp"
#include "config/GameConstants.hpp"

#include <engine/systems/MovementSystem.hpp>
#include <engine/systems/ScrollingBackgroundSystem.hpp>
#include <engine/systems/RenderSystem.hpp>
#include <engine/systems/EnemyHealthBarSystem.hpp>
#include <engine/systems/PlayerHealthBarSystem.hpp>
#include <engine/systems/BossHealthBarSystem.hpp>
#include <engine/systems/ScoreDisplaySystem.hpp>
#include <engine/systems/InputSystem.hpp>
#include <engine/systems/ShootingSystem.hpp>
#include <engine/systems/EnemySpawnSystem.hpp>
#include <engine/systems/EnemyShootingSystem.hpp>
#include <engine/systems/CollisionSystem.hpp>
#include <engine/systems/CleanupSystem.hpp>
#include <engine/systems/HealthRegenSystem.hpp>
#include <engine/graphics/Sprite.hpp>
#include <engine/physics/Transform.hpp>
#include <engine/physics/Velocity.hpp>
#include <engine/gameplay/Background.hpp>
#include <engine/gameplay/Controllable.hpp>
#include <engine/gameplay/Health.hpp>
#include <engine/gameplay/PlayerStats.hpp>
#include <engine/gameplay/Score.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>

void GameInitializer::initSystems(
    std::vector<std::unique_ptr<System>>& systems,
    float width,
    float height,
    IScoreProvider& scoreProvider,
    bool isSoloMode)
{
    systems.push_back(std::make_unique<MovementSystem>());
    systems.push_back(std::make_unique<ScrollingBackgroundSystem>(width));
    
    if (isSoloMode) {
        systems.push_back(std::make_unique<InputSystem>());
        systems.push_back(std::make_unique<ShootingSystem>(width));
        systems.push_back(std::make_unique<EnemySpawnSystem>(width, height));
        systems.push_back(std::make_unique<EnemyShootingSystem>());
        systems.push_back(std::make_unique<CollisionSystem>());
        systems.push_back(std::make_unique<HealthRegenSystem>());
        systems.push_back(std::make_unique<CleanupSystem>(width));
    }
    
    systems.push_back(std::make_unique<RenderSystem>());
    systems.push_back(std::make_unique<PlayerHealthBarSystem>());
    systems.push_back(std::make_unique<EnemyHealthBarSystem>());
    systems.push_back(std::make_unique<BossHealthBarSystem>());
    systems.push_back(std::make_unique<ScoreDisplaySystem>(scoreProvider));
}

void GameInitializer::initBackground(
    Registry& registry,
    float windowWidth,
    float windowHeight,
    const std::string& backgroundPath)
{
    using namespace GameConstants;

    float scaleX = windowWidth / BG_NATIVE_WIDTH;
    float scaleY = windowHeight / BG_NATIVE_HEIGHT;

    EntityID bg1 = registry.create();
    registry.add<Sprite>(bg1, backgroundPath);
    Transform& t1 = registry.add<Transform>(bg1, 0.f, 0.f);
    t1.scaleX = scaleX;
    t1.scaleY = scaleY;
    registry.add<Velocity>(bg1, BG_SCROLL_SPEED, 0.f);
    registry.add<Background>(bg1);

    EntityID bg2 = registry.create();
    registry.add<Sprite>(bg2, backgroundPath);
    Transform& t2 = registry.add<Transform>(bg2, windowWidth, 0.f);
    t2.scaleX = scaleX;
    t2.scaleY = scaleY;
    registry.add<Velocity>(bg2, BG_SCROLL_SPEED, 0.f);
    registry.add<Background>(bg2);
}

void GameInitializer::updateBackground(
    Registry& registry,
    const std::string& backgroundPath)
{
    std::cout << "[GameInitializer] Updating background to: " << backgroundPath << std::endl;
    registry.each<Background, Sprite>([&backgroundPath](EntityID, Background&, Sprite& sprite) {
        sprite.loadTexture(backgroundPath);
    });
}

void GameInitializer::initSoloPlayer(Registry& registry) {
    using namespace GameConstants;
    
    EntityID player = registry.create();
    
    Sprite& sprite = registry.add<Sprite>(player, "assets/sprites/player.png");
    sprite.getSprite().setTextureRect(sf::IntRect(
        PLAYER_SPRITE_RECT_X,
        PLAYER_SPRITE_RECT_Y,
        PLAYER_SPRITE_RECT_W,
        PLAYER_SPRITE_RECT_H
    ));
    
    Transform& transform = registry.add<Transform>(player, 100.0f, 360.0f);
    transform.scaleX = PLAYER_SCALE;
    transform.scaleY = PLAYER_SCALE;
    
    registry.add<Velocity>(player, 0.0f, 0.0f);
    
    Controllable& ctrl = registry.add<Controllable>(player);
    ctrl.speed = 300.0f;
    ctrl.canShoot = true;
    ctrl.currentCooldown = 0.0f;
    
    registry.add<Health>(player, 100.0f);
    registry.add<PlayerStats>(player);
    registry.add<Score>(player);
    
    std::cout << "[GameInitializer] Solo player created with upgrade system" << std::endl;
}

std::string GameInitializer::getPlayerTexture(uint8_t slot) {
    switch (slot) {
        case 0:
            return "assets/sprites/player.png";
        case 1:
            return "assets/sprites/player_pink.png";
        case 2:
            return "assets/sprites/player_green.png";
        case 3:
            return "assets/sprites/player_yellow.png";
        default: return "assets/sprites/player.png";
    }
}