#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>

class SpriteSheet {
private:
    std::shared_ptr<sf::Texture> _texture;
    std::vector<sf::IntRect> _frameRects;
    int _frameWidth;
    int _frameHeight;
    int _framesPerRow;

public:
    SpriteSheet();
    
    bool loadFromFile(const std::string& texturePath, int frameWidth, int frameHeight, int framesPerRow);
    bool loadFromFile(const std::string& texturePath, int totalWidth, int totalHeight, int cols, int rows);
    
    sf::IntRect getFrameRect(int frameIndex) const;
    int getFrameCount() const { return static_cast<int>(_frameRects.size()); }
    std::shared_ptr<sf::Texture> getTexture() const { return _texture; }
};