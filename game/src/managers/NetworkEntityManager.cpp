#include "managers/NetworkEntityManager.hpp"
#include "initialization/GameInitializer.hpp"
#include "config/GameConstants.hpp"
#include "managers/EntityTypes.hpp"
#include "helpers/GameClientHelpers.hpp"
#include "levels/Level2Renderer.hpp"
#include <engine/graphics/Sprite.hpp>
#include <engine/graphics/Animation.hpp>
#include <engine/graphics/SpriteSheet.hpp>
#include <engine/physics/Transform.hpp>
#include <engine/gameplay/Enemy.hpp>
#include <engine/gameplay/EnemyShooter.hpp>
#include <engine/gameplay/Health.hpp>
#include <cmath>
#include <iostream>

NetworkEntityManager::NetworkEntityManager(Registry& registry)
    : _registry(registry), _levelRenderer(nullptr)
{
}

void NetworkEntityManager::update(float dt, NetworkClient& network) {

    if (network.isSoloMode()) {
        return;
    }
    
    auto updates = network.pollEntityUpdates();

    for (const auto& update : updates) {
        auto it = _networkEntities.find(update.entity_id);

        if (it == _networkEntities.end()) {
            spawnEntity(update);
        } else {
            updateEntity(update, it->second.id);
        }
    }

    if (network.isServerResponding()) {
        despawnTimedOutEntities(dt);
    }
}

void NetworkEntityManager::spawnEntity(const ReceivedEntity& update) {
    using namespace GameConstants;
    
    EntityID local_entity = _registry.create();
    _networkEntities[update.entity_id] = { local_entity, 0.f };
    
    _registry.add<Transform>(local_entity, update.pos_x, update.pos_y);
    
    switch (RType::toEntityType(update.entity_type)) {
        case RType::EntityType::PLAYER: {
            std::string texturePath = GameInitializer::getPlayerTexture(update.player_slot);
            Sprite& sprite = _registry.add<Sprite>(local_entity, texturePath);
            sprite.getSprite().setTextureRect(sf::IntRect(
                PLAYER_SPRITE_RECT_X,
                PLAYER_SPRITE_RECT_Y,
                PLAYER_SPRITE_RECT_W,
                PLAYER_SPRITE_RECT_H
            ));
            
            Transform& t = _registry.get<Transform>(local_entity);
            t.scaleX = PLAYER_SCALE;
            t.scaleY = PLAYER_SCALE;
            
            float maxHp = update.hp_max > 0.f ? update.hp_max : PLAYER_DEFAULT_HP;
            Health& h = _registry.add<Health>(local_entity, maxHp);
            h.current = update.hp_current;
            break;
        }
        
        case RType::EntityType::ENEMY_BASIC: {
            std::cout << "[NetworkEntityManager] Spawning basic enemy ID=" << update.entity_id << std::endl;
            spawnEnemyOfType(local_entity, update, EnemyType::Basic, ENEMY_SCALE);
            break;
        }

        case RType::EntityType::ENEMY_FASTSHOOTER: {
            std::cout << "[NetworkEntityManager] Spawning FastShooter enemy ID=" << update.entity_id << std::endl;
            spawnEnemyOfType(local_entity, update, EnemyType::FastShooter, 1.3f);
            break;
        }

        case RType::EntityType::ENEMY_BOMBER: {
            std::cout << "[NetworkEntityManager] Spawning Bomber enemy ID=" << update.entity_id << std::endl;
            spawnEnemyOfType(local_entity, update, EnemyType::Bomber, 1.7f);
            break;
        }

        case RType::EntityType::BOSS: {
            std::cout << "[NetworkEntityManager] *** BOSS ENTITY SPAWNING *** ID=" << update.entity_id
                      << " HP=" << update.hp_current << "/" << update.hp_max
                      << " Pos=(" << update.pos_x << "," << update.pos_y << ")" << std::endl;

            const auto& frames = getBossFrames();
            std::cout << "[NetworkEntityManager] Boss animation frames available: " << frames.size() << std::endl;

            const std::string initialTexture = frames.empty()
                ? std::string("assets/sprites/boss_sprite/tile000.png")
                : frames.front();

            std::cout << "[NetworkEntityManager] Boss using initial texture: " << initialTexture << std::endl;
            Sprite& sprite = _registry.add<Sprite>(local_entity, initialTexture);
            std::cout << "[NetworkEntityManager] Boss sprite created, has texture: "
                      << (sprite.getSprite().getTexture() != nullptr) << std::endl;

            Transform& t = _registry.get<Transform>(local_entity);
            t.scaleX = 3.0f;
            t.scaleY = 3.0f;

            Animation& anim = _registry.add<Animation>(local_entity);
            anim.frameCount = static_cast<int>(frames.size());
            anim.currentFrame = 0;
            anim.frameTime = 0.2f;
            anim.elapsedTime = 0.f;
            anim.loop = true;

            _registry.add<Enemy>(local_entity, EnemyType::Boss);

            float maxHp = update.hp_max > 0.f ? update.hp_max : 10000.f;
            Health& h = _registry.add<Health>(local_entity, maxHp);
            h.current = update.hp_current;

            std::cout << "[NetworkEntityManager] ✓ Boss entity fully created with all components" << std::endl;
            break;
        }

        case RType::EntityType::PROJECTILE_NORMAL:
            _registry.add<Sprite>(local_entity, "assets/sprites/bullet_normal.png");
            break;
            
        case RType::EntityType::PROJECTILE_CHARGED:
            _registry.add<Sprite>(local_entity, "assets/sprites/bullet_charged.png");
            break;
            
        case RType::EntityType::PROJECTILE_ENEMY: {
            float speed = std::sqrt(update.vel_x * update.vel_x + update.vel_y * update.vel_y);

            if (speed > 260.f || std::abs(update.vel_y) > 1.f) {
                const auto& frames = getBossShotFrames();
                const std::string initialTexture = frames.empty()
                    ? std::string("assets/sprites/boss_shoot/tile000.png")
                    : frames.front();

                _registry.add<Sprite>(local_entity, initialTexture);
                Transform& t = _registry.get<Transform>(local_entity);
                t.scaleX = ENEMY_PROJECTILE_SCALE * 1.5f;
                t.scaleY = ENEMY_PROJECTILE_SCALE * 1.5f;

                Animation& anim = _registry.add<Animation>(local_entity);
                anim.frameCount = static_cast<int>(frames.size());
                anim.currentFrame = 0;
                anim.frameTime = 0.1f;
                anim.elapsedTime = 0.f;
                anim.loop = true;
            } else {
                _registry.add<Sprite>(local_entity, "assets/sprites/enemy-shoot.png");
                Transform& t = _registry.get<Transform>(local_entity);
                t.scaleX = ENEMY_PROJECTILE_SCALE;
                t.scaleY = ENEMY_PROJECTILE_SCALE;
            }
            break;
        }
    }
}

void NetworkEntityManager::spawnEnemyOfType(EntityID local_entity, const ReceivedEntity& update, 
                                             EnemyType type, float scale) {
    using namespace GameConstants;

    bool useSpriteSheet = false;
    const SpriteSheet* enemySheet = nullptr;
    float finalScale = scale;
    int spriteOffsetY = 0;
    
    if (_levelRenderer) {
        auto* level2Renderer = dynamic_cast<Level2Renderer*>(_levelRenderer);
        if (level2Renderer) {
            enemySheet = level2Renderer->getEnemySpriteSheet(type);
            if (enemySheet && enemySheet->getFrameCount() > 0) {
                useSpriteSheet = true;
                finalScale = level2Renderer->getEnemyScale(type);
                spriteOffsetY = level2Renderer->getEnemySpriteOffsetY(type);
                std::cout << "[NetworkEntityManager] Using sprite sheet for " 
                          << getEnemyTypeName(type) << " (Level 2)" << std::endl;
            }
        }
    }

    if (useSpriteSheet && enemySheet) {
        Sprite& sprite = _registry.add<Sprite>(local_entity);
        sprite.setTexture(enemySheet->getTexture());
        
        int startFrame = 10;
        sf::IntRect frameRect = enemySheet->getFrameRect(startFrame);
        frameRect.top += spriteOffsetY;
        sprite.getSprite().setTextureRect(frameRect);

        Transform& t = _registry.get<Transform>(local_entity);
        t.scaleX = finalScale;
        t.scaleY = finalScale;

        Animation& anim = _registry.add<Animation>(local_entity);
        anim.frameCount = 5;
        anim.currentFrame = 0;
        anim.frameTime = 0.15f;
        anim.elapsedTime = 0.f;
        anim.loop = true;
        
        for (int i = 0; i < 5; ++i) {
            sf::IntRect rect = enemySheet->getFrameRect(startFrame + i);
            rect.top += spriteOffsetY;
            anim.addFrame(rect.left, rect.top, rect.width, rect.height);
        }

        std::cout << "[NetworkEntityManager] ✓ " << getEnemyTypeName(type) 
                  << " created with sprite sheet (5 frames, scale=" << finalScale 
                  << ", offsetY=" << spriteOffsetY << ")" << std::endl;
    } else {
        const auto& frames = getEnemyBasicFrames();
        const std::string initialTexture = frames.empty()
            ? std::string("assets/sprites/enemy_basic.png")
            : frames.front();

        _registry.add<Sprite>(local_entity, initialTexture);

        Transform& t = _registry.get<Transform>(local_entity);
        t.scaleX = scale;
        t.scaleY = scale;

        Animation& anim = _registry.add<Animation>(local_entity);
        anim.frameCount = static_cast<int>(frames.size());
        anim.currentFrame = 0;
        anim.frameTime = 0.2f;
        anim.elapsedTime = 0.f;
        anim.loop = true;

        std::cout << "[NetworkEntityManager] ✓ " << getEnemyTypeName(type) 
                  << " created with fallback frames (" << frames.size() << " frames)" << std::endl;
    }

    _registry.add<Enemy>(local_entity, type);
    _registry.add<EnemyShooter>(local_entity, EnemyShooter::createForType(type));

    float maxHp = update.hp_max > 0.f ? update.hp_max : ENEMY_DEFAULT_HP;
    Health& h = _registry.add<Health>(local_entity, maxHp);
    h.current = update.hp_current;
}

const char* NetworkEntityManager::getEnemyTypeName(EnemyType type) const {
    switch (type) {
        case EnemyType::Basic: return "Basic Enemy";
        case EnemyType::FastShooter: return "FastShooter";
        case EnemyType::Bomber: return "Bomber";
        case EnemyType::Boss: return "Boss";
        default: return "Unknown";
    }
}

void NetworkEntityManager::updateEntity(const ReceivedEntity& update, EntityID localEntity) {
    using namespace GameConstants;
    
    auto it = _networkEntities.find(update.entity_id);
    if (it != _networkEntities.end()) {
        it->second.lastSeen = 0.f;
    }
    
    if (_registry.has<Transform>(localEntity)) {
        Transform& transform = _registry.get<Transform>(localEntity);
        transform.x = update.pos_x;
        transform.y = update.pos_y;
    }

    auto entityType = RType::toEntityType(update.entity_type);
    if ((entityType == RType::EntityType::PLAYER ||
         entityType == RType::EntityType::ENEMY_BASIC ||
         entityType == RType::EntityType::ENEMY_FASTSHOOTER ||
         entityType == RType::EntityType::ENEMY_BOMBER ||
         entityType == RType::EntityType::BOSS) &&
        update.hp_max > 0.f) {
        Health& h = _registry.has<Health>(localEntity)
            ? _registry.get<Health>(localEntity)
            : _registry.add<Health>(localEntity, update.hp_max);
        h.max = update.hp_max;
        h.current = update.hp_current;
    }
}

void NetworkEntityManager::despawnTimedOutEntities(float dt) {
    using namespace GameConstants;
    
    for (auto it = _networkEntities.begin(); it != _networkEntities.end(); ) {
        it->second.lastSeen += dt;
        if (it->second.lastSeen > DESPAWN_TIMEOUT) {
            _registry.markForDestruction(it->second.id);
            it = _networkEntities.erase(it);
        } else {
            ++it;
        }
    }
}

void NetworkEntityManager::clearAll() {
    for (auto& [net_id, info] : _networkEntities) {
        _registry.markForDestruction(info.id);
    }
    _networkEntities.clear();
    _registry.cleanup();
}