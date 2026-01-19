#include "engine/systems/RenderSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Renderer.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Enemy.hpp"
#include <iostream>

RenderSystem::RenderSystem() = default;

void RenderSystem::update(Registry&, float)
{
}

void RenderSystem::render(Registry& registry, Renderer& renderer)
{
    static int frameCount = 0;
    int entityCount = 0;
    int bossCount = 0;

    registry.each<Transform, Sprite>([&](EntityID id, Transform& t, Sprite& s) {
        s.setPosition(t.x, t.y);
        s.setScale(t.scaleX, t.scaleY);

        bool hasTexture = (s.getSprite().getTexture() != nullptr);
        sf::FloatRect bounds = s.getSprite().getGlobalBounds();

        if (registry.has<Enemy>(id)) {
            auto& enemy = registry.get<Enemy>(id);
            if (enemy.type == EnemyType::Boss) {
                bossCount++;
                if (frameCount % 60 == 0) {
                    std::cout << "[RenderSystem] Boss entity " << id << " at (" << t.x << "," << t.y
                              << ") scale(" << t.scaleX << "," << t.scaleY << ") hasTexture=" << hasTexture
                              << " bounds=(" << bounds.left << "," << bounds.top << "," << bounds.width << "," << bounds.height << ")" << std::endl;
                }
            }
        }

        renderer.getWindow().draw(s.getSprite());
        entityCount++;
    });

    if (frameCount % 60 == 0) {
        std::cout << "[RenderSystem] Rendering " << entityCount << " entities (boss: " << bossCount << ") at frame " << frameCount << std::endl;
    }
    frameCount++;
}