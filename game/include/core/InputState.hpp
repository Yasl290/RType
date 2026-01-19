#pragma once

#include <SFML/Window/Keyboard.hpp>
#include <cstdint>

class InputState {
public:
    InputState() = default;
    
    void setKey(sf::Keyboard::Key key, bool pressed);
    void reset();
    uint8_t toFlags() const;
    
    bool isUp() const { return _up; }
    bool isDown() const { return _down; }
    bool isLeft() const { return _left; }
    bool isRight() const { return _right; }
    bool isShoot() const { return _shoot; }

private:
    bool _up = false;
    bool _down = false;
    bool _left = false;
    bool _right = false;
    bool _shoot = false;
};