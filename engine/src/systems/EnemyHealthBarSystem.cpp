#include "engine/systems/EnemyHealthBarSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Renderer.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Enemy.hpp"
#include "engine/gameplay/Health.hpp"
#include <SFML/Graphics/RectangleShape.hpp>

EnemyHealthBarSystem::EnemyHealthBarSystem() = default;

void EnemyHealthBarSystem::update(Registry&, float)
{
}

void EnemyHealthBarSystem::render(Registry& registry, Renderer& renderer)
{
    auto& window = renderer.getWindow();

    registry.each<Transform, Sprite, Enemy, Health>(
        [&](EntityID, Transform& t, Sprite& s, Enemy& e, Health& h) {
            if (e.type == EnemyType::Boss) {
                return;
            }
            if (h.max <= 0.f) {
                return;
            }

            float ratio = h.current / h.max;
            if (ratio < 0.f) ratio = 0.f;
            if (ratio > 1.f) ratio = 1.f;

            s.setPosition(t.x, t.y);
            s.setScale(t.scaleX, t.scaleY);
            sf::FloatRect bounds = s.getSprite().getGlobalBounds();

            const float fullWidth = bounds.width;
            const float barHeight = 5.f;
            const float offsetY = 6.f;

            float barX = bounds.left;
            float barY = bounds.top - offsetY - barHeight;

            sf::RectangleShape background(sf::Vector2f(fullWidth, barHeight));
            background.setFillColor(sf::Color(80, 0, 0));
            background.setPosition(barX, barY);

            sf::RectangleShape foreground(sf::Vector2f(fullWidth * ratio, barHeight));
            foreground.setFillColor(sf::Color(0, 220, 0));
            foreground.setPosition(barX, barY);

            window.draw(background);
            window.draw(foreground);
        });
}