#include "engine/systems/RenderSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Renderer.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/physics/Transform.hpp"

RenderSystem::RenderSystem() = default;

void RenderSystem::update(Registry&, float)
{
}

void RenderSystem::render(Registry& registry, Renderer& renderer)
{
    registry.each<Transform, Sprite>([&renderer](EntityID, Transform& t, Sprite& s) {
        s.setPosition(t.x, t.y);
        s.setScale(t.scaleX, t.scaleY);
        renderer.getWindow().draw(s.getSprite());
    });
}