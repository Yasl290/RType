#include "engine/systems/ServerInputSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/PlayerInputState.hpp"

ServerInputSystem::ServerInputSystem() = default;

void ServerInputSystem::update(Registry& registry, float deltaTime)
{
    registry.each<Velocity, Controllable, PlayerInputState>(
        [deltaTime](EntityID, Velocity& v, Controllable& c, PlayerInputState& input) {
            v.x = 0.f;
            v.y = 0.f;

            uint8_t flags = input.inputFlags;

            constexpr uint8_t MOVE_LEFT  = 1u << 0;
            constexpr uint8_t MOVE_RIGHT = 1u << 1;
            constexpr uint8_t MOVE_UP    = 1u << 2;
            constexpr uint8_t MOVE_DOWN  = 1u << 3;

            if (flags & MOVE_LEFT) {
                v.x = -c.speed;
            }
            if (flags & MOVE_RIGHT) {
                v.x = c.speed;
            }
            if (flags & MOVE_UP) {
                v.y = -c.speed;
            }
            if (flags & MOVE_DOWN) {
                v.y = c.speed;
            }

            if (c.currentCooldown > 0.f) {
                c.currentCooldown -= deltaTime;
            }
        });
}