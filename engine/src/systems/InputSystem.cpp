#include "engine/systems/InputSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Controllable.hpp"
#include <SFML/Window/Keyboard.hpp>

InputSystem::InputSystem() = default;

void InputSystem::update(Registry& registry, float deltaTime)
{
    registry.each<Velocity, Controllable>([deltaTime](EntityID, Velocity& v, Controllable& c) {
        v.x = 0.f;
        v.y = 0.f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
            v.x = -c.speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            v.x = c.speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) {
            v.y = -c.speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            v.y = c.speed;
        }

        if (c.currentCooldown > 0.f) {
            c.currentCooldown -= deltaTime;
        }
    });
}