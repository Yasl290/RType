#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>

class EndlessGameOverScreen {
public:
    EndlessGameOverScreen();
    
    void show(uint32_t finalScore, uint32_t finalKills, 
              uint32_t highscore, uint32_t bestKills, bool isNewRecord);
    void hide();
    void render(sf::RenderWindow& window);
    void onResize(const sf::Vector2u& newSize);
    
    enum class Action {
        NONE,
        RESTART,
        MENU
    };
    
    Action handleEvent(const sf::Event& event);
    
private:
    void updateLayout();
    
    sf::Font _font;
    sf::Text _gameOverText;
    sf::Text _scoreText;
    sf::Text _killsText;
    sf::Text _highscoreText;
    sf::Text _bestKillsText;
    sf::Text _newRecordText;
    sf::Text _restartText;
    sf::Text _menuText;
    
    sf::RectangleShape _overlay;
    sf::RectangleShape _restartButton;
    sf::RectangleShape _menuButton;
    
    sf::Vector2u _windowSize;
    bool _visible;
    bool _isNewRecord;
};