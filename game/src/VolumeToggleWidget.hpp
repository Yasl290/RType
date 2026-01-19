#pragma once

#include <SFML/Graphics.hpp>
#include <iostream>

class VolumeToggleWidget {
public:
    VolumeToggleWidget()
    {
        if (!_texOn.loadFromFile("assets/sprites/sound_on.png")) {
            std::cerr << "Failed to load sound_on.png" << std::endl;
        }
        if (!_texOff.loadFromFile("assets/sprites/sound_off.png")) {
            std::cerr << "Failed to load sound_off.png" << std::endl;
        }
    }

    void setPosition(sf::Vector2f pos)
    {
        _pos = pos;
        rebuild();
    }

    void setSize(float sizePx)
    {
        _size = sizePx;
        rebuild();
    }

    void setMuted(bool muted)
    {
        _muted = muted;
        rebuild();
    }

    bool isMuted() const
    {
        return _muted;
    }

    sf::FloatRect getBounds() const
    {
        return {_pos.x, _pos.y, _size, _size};
    }

    bool hitTest(sf::Vector2i mousePos) const
    {
        return getBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
    }

    void draw(sf::RenderTarget& target) const
    {
        target.draw(_sprite);
    }

    void setColor(sf::Color color)
    {
        _color = color;
        _sprite.setColor(_color);
    }

private:
    void rebuild()
    {
        const sf::Texture* tex = _muted ? &_texOff : &_texOn;
        _sprite.setTexture(*tex);

        sf::Vector2u texSize = tex->getSize();
        if (texSize.x > 0 && texSize.y > 0) {
            float scaleX = _size / static_cast<float>(texSize.x);
            float scaleY = _size / static_cast<float>(texSize.y);
            _sprite.setScale(scaleX, scaleY);
        }

        _sprite.setPosition(_pos);
        _sprite.setColor(_color);
    }

    sf::Vector2f _pos{0.f, 0.f};
    float _size = 40.f;
    bool _muted = false;

    sf::Color _color = sf::Color::White;

    sf::Texture _texOn;
    sf::Texture _texOff;
    sf::Sprite _sprite;
};
