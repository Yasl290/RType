#include "engine/systems/BoundarySystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/graphics/Sprite.hpp"

BoundarySystem::BoundarySystem(float minX, float maxX, float minY, float maxY)
    : _minX(minX), _maxX(maxX), _minY(minY), _maxY(maxY)
{
}

void BoundarySystem::update(Registry& registry, float)
{
    registry.each<Transform, Controllable, Sprite>([this](EntityID, Transform& t, Controllable&, Sprite& sprite) {
        sprite.setPosition(t.x, t.y);
        sprite.setScale(t.scaleX, t.scaleY);
        sf::FloatRect bounds = sprite.getSprite().getGlobalBounds();

        float spriteWidth = bounds.width;
        float spriteHeight = bounds.height;

        if (t.x < _minX)
            t.x = _minX;
        if (t.x + spriteWidth > _maxX)
            t.x = _maxX - spriteWidth;
        if (t.y < _minY)
            t.y = _minY;
        if (t.y + spriteHeight > _maxY)
            t.y = _maxY - spriteHeight;
    });
}