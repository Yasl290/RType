#include "engine/systems/PlayerHealthBarSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Renderer.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Enemy.hpp"
#include <SFML/Graphics/RectangleShape.hpp>

PlayerHealthBarSystem::PlayerHealthBarSystem() = default;

void PlayerHealthBarSystem::update(Registry&, float)
{
}

void PlayerHealthBarSystem::render(Registry& registry, Renderer& renderer)
{
    auto& window = renderer.getWindow();

    registry.each<Transform, Sprite, Health>(
        [&](EntityID id, Transform& t, Sprite& s, Health& h) {
            if (registry.has<Enemy>(id)) {
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
            const float barHeight = 6.f;
            const float offsetY = 10.f;

            float barX = bounds.left;
            float barY = bounds.top - offsetY - barHeight;

            sf::RectangleShape background(sf::Vector2f(fullWidth, barHeight));
            background.setFillColor(sf::Color(0, 0, 0, 180));
            background.setPosition(barX, barY);

            sf::RectangleShape foreground(sf::Vector2f(fullWidth * ratio, barHeight));
            foreground.setFillColor(sf::Color(0, 200, 255));
            foreground.setPosition(barX, barY);

            window.draw(background);
            window.draw(foreground);
        });
}
