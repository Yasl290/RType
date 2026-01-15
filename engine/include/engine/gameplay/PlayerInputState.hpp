#pragma once

#include "engine/core/Component.hpp"
#include <cstdint>

class PlayerInputState : public Component {
public:
    PlayerInputState();
    explicit PlayerInputState(uint8_t flags);
    ~PlayerInputState() override = default;

    uint8_t inputFlags;
};