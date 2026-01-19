#include "engine/graphics/SpriteSheet.hpp"
#include <iostream>

SpriteSheet::SpriteSheet()
    : _frameWidth(0), _frameHeight(0), _framesPerRow(0)
{
}

bool SpriteSheet::loadFromFile(const std::string& texturePath, int frameWidth, int frameHeight, int framesPerRow)
{
    _frameWidth = frameWidth;
    _frameHeight = frameHeight;
    _framesPerRow = framesPerRow;
    
    _texture = std::make_shared<sf::Texture>();
    if (!_texture->loadFromFile(texturePath)) {
        std::cerr << "[SpriteSheet] Failed to load texture: " << texturePath << std::endl;
        return false;
    }
    
    int textureWidth = _texture->getSize().x;
    int textureHeight = _texture->getSize().y;
    
    int rows = textureHeight / frameHeight;
    int cols = textureWidth / frameWidth;
    
    _frameRects.clear();
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < framesPerRow && col < cols; ++col) {
            sf::IntRect rect(
                col * frameWidth,
                row * frameHeight,
                frameWidth,
                frameHeight
            );
            _frameRects.push_back(rect);
        }
    }
    
    std::cout << "[SpriteSheet] Loaded " << _frameRects.size() << " frames from " << texturePath << std::endl;
    return true;
}

bool SpriteSheet::loadFromFile(const std::string& texturePath, int totalWidth, int totalHeight, int cols, int rows)
{
    _texture = std::make_shared<sf::Texture>();
    if (!_texture->loadFromFile(texturePath)) {
        std::cerr << "[SpriteSheet] Failed to load texture: " << texturePath << std::endl;
        return false;
    }
    
    _frameWidth = totalWidth / cols;
    _frameHeight = totalHeight / rows;
    _framesPerRow = cols;
    
    _frameRects.clear();
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            sf::IntRect rect(
                col * _frameWidth,
                row * _frameHeight,
                _frameWidth,
                _frameHeight
            );
            _frameRects.push_back(rect);
        }
    }
    
    std::cout << "[SpriteSheet] Loaded " << _frameRects.size() << " frames from " << texturePath 
              << " (frame size: " << _frameWidth << "x" << _frameHeight << ")" << std::endl;
    return true;
}

sf::IntRect SpriteSheet::getFrameRect(int frameIndex) const
{
    if (frameIndex >= 0 && frameIndex < static_cast<int>(_frameRects.size())) {
        return _frameRects[frameIndex];
    }
    return sf::IntRect(0, 0, _frameWidth, _frameHeight);
}