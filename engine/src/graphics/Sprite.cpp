#include "engine/graphics/Sprite.hpp"
#include <iostream>
#include <unordered_map>

static std::unordered_map<std::string, std::shared_ptr<sf::Texture>> g_textureCache;

Sprite::Sprite() : texture(nullptr)
{
}

Sprite::Sprite(const std::string& texturePath) : texture(nullptr)
{
    loadTexture(texturePath);
}

Sprite::~Sprite() = default;

bool Sprite::loadTexture(const std::string& path)
{
    auto it = g_textureCache.find(path);
    if (it != g_textureCache.end()) {
        texture = it->second;
        sprite.setTexture(*texture, true);
        return true;
    }

    auto newTexture = std::make_shared<sf::Texture>();
    if (!newTexture->loadFromFile(path)) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return false;
    }

    g_textureCache[path] = newTexture;
    texture = newTexture;
    sprite.setTexture(*texture, true);

    return true;
}

void Sprite::setScale(float scaleX, float scaleY)
{
    sprite.setScale(scaleX, scaleY);
}

void Sprite::setPosition(float x, float y)
{
    sprite.setPosition(x, y);
}

sf::Sprite& Sprite::getSprite()
{
    return sprite;
}

const sf::Sprite& Sprite::getSprite() const
{
    return sprite;
}

void Sprite::clearTextureCache()
{
    g_textureCache.clear();
}