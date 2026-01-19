#include "core/InputState.hpp"

void InputState::setKey(sf::Keyboard::Key key, bool pressed) {
    switch (key) {
        case sf::Keyboard::Up:
        case sf::Keyboard::Z:
            _up = pressed;
            break;
        case sf::Keyboard::Down:
        case sf::Keyboard::S:
            _down = pressed;
            break;
        case sf::Keyboard::Left:
        case sf::Keyboard::Q:
            _left = pressed;
            break;
        case sf::Keyboard::Right:
        case sf::Keyboard::D:
            _right = pressed;
            break;
        case sf::Keyboard::Space:
            _shoot = pressed;
            break;
        default:
            break;
    }
}

void InputState::reset() {
    _up = _down = _left = _right = _shoot = false;
}

uint8_t InputState::toFlags() const {
    uint8_t flags = 0;
    if (_up)
        flags |= 0x01;
    if (_down)
        flags |= 0x02;
    if (_left)
        flags |= 0x04;
    if (_right)
        flags |= 0x08;
    if (_shoot)
        flags |= 0x10;
    return flags;
}