#include "engine/systems/ScrollingBackgroundSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/gameplay/Background.hpp"

ScrollingBackgroundSystem::ScrollingBackgroundSystem(float windowWidth)
    : _windowWidth(windowWidth)
{
}

void ScrollingBackgroundSystem::update(Registry& registry, float)
{
    registry.each<Transform, Background>([this](EntityID, Transform& t, Background&) {
        if (t.x + _windowWidth <= 0.f) {
            t.x += _windowWidth * 2.f;
        }
    });
}