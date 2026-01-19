#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class LevelTransition {
public:
    LevelTransition();
    
    void show(const std::string& currentLevelName, const std::string& nextLevelName);
    void hide();
    bool isVisible() const { return _visible; }
    
    void update(float deltaTime);
    void render(sf::RenderWindow& window);
    void onResize(const sf::Vector2u& windowSize);
    
    bool isComplete() const { return _elapsed >= _duration; }
    
    void setDuration(float seconds) { _duration = seconds; }

private:
    bool _visible;
    float _elapsed;
    float _duration;
    
    sf::Font _font;
    sf::RectangleShape _background;
    sf::Text _completedText;
    sf::Text _nextLevelText;
    sf::Text _preparingText;
    
    sf::Vector2u _windowSize;
    
    void updateAnimation();
};