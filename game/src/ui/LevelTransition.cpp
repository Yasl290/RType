#include "ui/LevelTransition.hpp"
#include <iostream>
#include <cmath>

LevelTransition::LevelTransition()
    : _visible(false)
    , _elapsed(0.0f)
    , _duration(3.0f)
    , _windowSize(1280, 720)
{
    if (!_font.loadFromFile("assets/font/star-crush/Star_Crush.ttf")) {
        std::cerr << "[LevelTransition] Failed to load font" << std::endl;
    }
    
    _background.setFillColor(sf::Color(0, 0, 0, 230));
    
    _completedText.setFont(_font);
    _completedText.setCharacterSize(60);
    _completedText.setFillColor(sf::Color(0, 255, 0));
    _completedText.setOutlineColor(sf::Color::Black);
    _completedText.setOutlineThickness(3);
    
    _nextLevelText.setFont(_font);
    _nextLevelText.setCharacterSize(50);
    _nextLevelText.setFillColor(sf::Color(255, 215, 0));
    _nextLevelText.setOutlineColor(sf::Color::Black);
    _nextLevelText.setOutlineThickness(2);
    
    _preparingText.setFont(_font);
    _preparingText.setString("Preparing for next level...");
    _preparingText.setCharacterSize(30);
    _preparingText.setFillColor(sf::Color::White);
}

void LevelTransition::show(const std::string& currentLevelName, const std::string& nextLevelName) {
    _visible = true;
    _elapsed = 0.0f;
    
    _completedText.setString(currentLevelName + " COMPLETE!");
    _nextLevelText.setString("Next: " + nextLevelName);
    
    onResize(_windowSize);
    
    std::cout << "[LevelTransition] Showing transition: " << currentLevelName 
              << " -> " << nextLevelName << std::endl;
}

void LevelTransition::hide() {
    _visible = false;
    _elapsed = 0.0f;
}

void LevelTransition::update(float deltaTime) {
    if (!_visible) return;
    
    _elapsed += deltaTime;
    updateAnimation();
}

void LevelTransition::updateAnimation() {
    float pulse = 0.5f + 0.5f * std::sin(_elapsed * 3.0f);
    sf::Color prepColor = _preparingText.getFillColor();
    prepColor.a = static_cast<sf::Uint8>(150 + 105 * pulse);
    _preparingText.setFillColor(prepColor);
}

void LevelTransition::render(sf::RenderWindow& window) {
    if (!_visible) return;
    
    window.draw(_background);
    window.draw(_completedText);
    window.draw(_nextLevelText);
    window.draw(_preparingText);
}

void LevelTransition::onResize(const sf::Vector2u& windowSize) {
    _windowSize = windowSize;
    float width = static_cast<float>(windowSize.x);
    float height = static_cast<float>(windowSize.y);
    
    _background.setSize(sf::Vector2f(width, height));
    
    sf::FloatRect completedBounds = _completedText.getLocalBounds();
    _completedText.setPosition(
        (width - completedBounds.width) / 2.0f,
        height * 0.35f
    );
    
    sf::FloatRect nextBounds = _nextLevelText.getLocalBounds();
    _nextLevelText.setPosition(
        (width - nextBounds.width) / 2.0f,
        height * 0.50f
    );
    
    sf::FloatRect prepBounds = _preparingText.getLocalBounds();
    _preparingText.setPosition(
        (width - prepBounds.width) / 2.0f,
        height * 0.65f
    );
}