#include "engine/gameplay/PlayerInputState.hpp"

PlayerInputState::PlayerInputState()
    : inputFlags(0)
{
}

PlayerInputState::PlayerInputState(uint8_t flags)
    : inputFlags(flags)
{
}