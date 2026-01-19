#include "ui/EndlessGameOverScreen.hpp"
#include <iostream>

EndlessGameOverScreen::EndlessGameOverScreen()
    : _windowSize(1280, 720)
    , _visible(false)
    , _isNewRecord(false)
{
    if (!_font.loadFromFile("assets/font/star-crush/Star_Crush.ttf")) {
        std::cerr << "[EndlessGameOverScreen] Failed to load font" << std::endl;
    }
    
    _overlay.setFillColor(sf::Color(0, 0, 0, 200));
    
    _gameOverText.setFont(_font);
    _gameOverText.setString("GAME OVER");
    _gameOverText.setCharacterSize(80);
    _gameOverText.setFillColor(sf::Color::Red);
    
    _scoreText.setFont(_font);
    _scoreText.setCharacterSize(40);
    _scoreText.setFillColor(sf::Color::White);
    
    _killsText.setFont(_font);
    _killsText.setCharacterSize(40);
    _killsText.setFillColor(sf::Color::White);
    
    _highscoreText.setFont(_font);
    _highscoreText.setCharacterSize(30);
    _highscoreText.setFillColor(sf::Color(255, 215, 0));
    _bestKillsText.setFont(_font);
    _bestKillsText.setCharacterSize(30);
    _bestKillsText.setFillColor(sf::Color(255, 215, 0));
    
    _newRecordText.setFont(_font);
    _newRecordText.setString(" NEW RECORD! ");
    _newRecordText.setCharacterSize(50);
    _newRecordText.setFillColor(sf::Color(255, 215, 0));
    
    _restartButton.setSize(sf::Vector2f(200, 60));
    _restartButton.setFillColor(sf::Color(50, 50, 50));
    _restartButton.setOutlineColor(sf::Color::White);
    _restartButton.setOutlineThickness(2);
    
    _menuButton.setSize(sf::Vector2f(200, 60));
    _menuButton.setFillColor(sf::Color(50, 50, 50));
    _menuButton.setOutlineColor(sf::Color::White);
    _menuButton.setOutlineThickness(2);
    
    _restartText.setFont(_font);
    _restartText.setString("RESTART");
    _restartText.setCharacterSize(30);
    _restartText.setFillColor(sf::Color::White);
    
    _menuText.setFont(_font);
    _menuText.setString("MENU");
    _menuText.setCharacterSize(30);
    _menuText.setFillColor(sf::Color::White);
    
    updateLayout();
}

void EndlessGameOverScreen::show(uint32_t finalScore, uint32_t finalKills, 
                                 uint32_t highscore, uint32_t bestKills, bool isNewRecord) {
    _visible = true;
    _isNewRecord = isNewRecord;
    _scoreText.setString("Score: " + std::to_string(finalScore));
    _killsText.setString("Kills: " + std::to_string(finalKills));
    _highscoreText.setString("Highscore: " + std::to_string(highscore));
    _bestKillsText.setString("Best Kills: " + std::to_string(bestKills));
    
    updateLayout();
}

void EndlessGameOverScreen::hide() {
    _visible = false;
}

void EndlessGameOverScreen::onResize(const sf::Vector2u& newSize) {
    _windowSize = newSize;
    updateLayout();
}

void EndlessGameOverScreen::updateLayout() {
    _overlay.setSize(sf::Vector2f(_windowSize.x, _windowSize.y));
    
    float centerX = _windowSize.x / 2.0f;
    float startY = _windowSize.y / 2.0f - 200.0f;
    

    sf::FloatRect bounds = _gameOverText.getLocalBounds();
    _gameOverText.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    _gameOverText.setPosition(centerX, startY);
    

    if (_isNewRecord) {
        bounds = _newRecordText.getLocalBounds();
        _newRecordText.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
        _newRecordText.setPosition(centerX, startY + 80);
        startY += 60;
    }
    

    bounds = _scoreText.getLocalBounds();
    _scoreText.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    _scoreText.setPosition(centerX, startY + 120);
    

    bounds = _killsText.getLocalBounds();
    _killsText.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    _killsText.setPosition(centerX, startY + 180);
    

    bounds = _highscoreText.getLocalBounds();
    _highscoreText.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    _highscoreText.setPosition(centerX, startY + 240);
    

    bounds = _bestKillsText.getLocalBounds();
    _bestKillsText.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    _bestKillsText.setPosition(centerX, startY + 290);
    

    _restartButton.setPosition(centerX - 220, startY + 360);
    _menuButton.setPosition(centerX + 20, startY + 360);
    
    bounds = _restartText.getLocalBounds();
    _restartText.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    _restartText.setPosition(centerX - 120, startY + 390);
    
    bounds = _menuText.getLocalBounds();
    _menuText.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    _menuText.setPosition(centerX + 120, startY + 390);
}

EndlessGameOverScreen::Action EndlessGameOverScreen::handleEvent(const sf::Event& event) {
    if (!_visible) {
        return Action::NONE;
    }
    
    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
        
        if (_restartButton.getGlobalBounds().contains(mousePos)) {
            return Action::RESTART;
        }
        
        if (_menuButton.getGlobalBounds().contains(mousePos)) {
            return Action::MENU;
        }
    }
    
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::R) {
            return Action::RESTART;
        }
        if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::M) {
            return Action::MENU;
        }
    }
    
    return Action::NONE;
}

void EndlessGameOverScreen::render(sf::RenderWindow& window) {
    if (!_visible) {
        return;
    }
    
    window.draw(_overlay);
    window.draw(_gameOverText);
    
    if (_isNewRecord) {
        window.draw(_newRecordText);
    }
    
    window.draw(_scoreText);
    window.draw(_killsText);
    window.draw(_highscoreText);
    window.draw(_bestKillsText);
    
    window.draw(_restartButton);
    window.draw(_menuButton);
    window.draw(_restartText);
    window.draw(_menuText);
}