#include "ui/GameOverScreen.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>

GameOverScreen::GameOverScreen()
    : _selectedOption(0)
    , _windowSize(1280, 720)
{
    loadFont();
    
    _titleText.setFont(_font);
    _titleText.setString("GAME OVER");
    _titleText.setCharacterSize(72);
    _titleText.setFillColor(sf::Color::Red);
    _titleText.setStyle(sf::Text::Bold);
    
    _restartText.setFont(_font);
    _restartText.setString("RESTART");
    _restartText.setCharacterSize(36);
    
    _menuText.setFont(_font);
    _menuText.setString("MAIN MENU");
    _menuText.setCharacterSize(36);
    
    _quitText.setFont(_font);
    _quitText.setString("QUIT");
    _quitText.setCharacterSize(36);
    
    updateLayout();
}

void GameOverScreen::loadFont()
{
    if (!_font.loadFromFile("assets/font/star-crush/Star_Crush.ttf")) {
        std::cerr << "[GameOverScreen] Failed to load font" << std::endl;
    }
}

void GameOverScreen::setScores(const std::vector<RType::Protocol::PlayerFinalScore>& scores)
{
    _scores = scores;
    sortScoresByRank();
    
    _scoreTexts.clear();
    
    for (size_t i = 0; i < _scores.size(); ++i) {
        const auto& score = _scores[i];
        
        sf::Text text;
        text.setFont(_font);
        
        std::string rank;
        switch (i) {
            case 0: rank = "1st"; break;
            case 1: rank = "2nd"; break;
            case 2: rank = "3rd"; break;
            default: rank = std::to_string(i + 1) + "th"; break;
        }
        
        std::string playerName(score.player_name);
        text.setString(rank + " - " + playerName + 
                       ": " + std::to_string(score.score) + " pts (" + 
                       std::to_string(score.enemies_killed) + " kills)");
        text.setCharacterSize(32);
        text.setFillColor(i == 0 ? sf::Color::Yellow : sf::Color::White);
        
        _scoreTexts.push_back(text);
    }
    
    updateLayout();
}

void GameOverScreen::sortScoresByRank()
{
    std::sort(_scores.begin(), _scores.end(), 
              [](const RType::Protocol::PlayerFinalScore& a, 
                 const RType::Protocol::PlayerFinalScore& b) {
                  return a.score > b.score;
              });
}

void GameOverScreen::updateLayout()
{
    float centerX = _windowSize.x / 2.f;
    float centerY = _windowSize.y / 2.f;
    
    sf::FloatRect titleBounds = _titleText.getLocalBounds();
    _titleText.setPosition(centerX - titleBounds.width / 2.f, 50.f);
    
    float scoreStartY = 180.f;
    for (size_t i = 0; i < _scoreTexts.size(); ++i) {
        sf::FloatRect bounds = _scoreTexts[i].getLocalBounds();
        _scoreTexts[i].setPosition(centerX - bounds.width / 2.f, 
                                   scoreStartY + i * 50.f);
    }
    
    float optionsStartY = centerY + 100.f;
    
    sf::FloatRect restartBounds = _restartText.getLocalBounds();
    _restartText.setPosition(centerX - restartBounds.width / 2.f, optionsStartY);
    
    sf::FloatRect menuBounds = _menuText.getLocalBounds();
    _menuText.setPosition(centerX - menuBounds.width / 2.f, optionsStartY + 60.f);
    
    sf::FloatRect quitBounds = _quitText.getLocalBounds();
    _quitText.setPosition(centerX - quitBounds.width / 2.f, optionsStartY + 120.f);
}

GameOverAction GameOverScreen::handleEvent(const sf::Event& event)
{
    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Up:
            case sf::Keyboard::Z:
                _selectedOption = (_selectedOption - 1 + 3) % 3;
                break;

            case sf::Keyboard::Down:
            case sf::Keyboard::S:
                _selectedOption = (_selectedOption + 1) % 3;
                break;

            case sf::Keyboard::Return:
            case sf::Keyboard::Space:
                switch (_selectedOption) {
                    case 0: return GameOverAction::RESTART;
                    case 1: return GameOverAction::RETURN_TO_MENU;
                    case 2: return GameOverAction::QUIT;
                }
                break;

            default:
                break;
        }
    }


    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);

        if (isMouseOver(_restartText, mousePos)) {
            return GameOverAction::RESTART;
        }
        if (isMouseOver(_menuText, mousePos)) {
            return GameOverAction::RETURN_TO_MENU;
        }
        if (isMouseOver(_quitText, mousePos)) {
            return GameOverAction::QUIT;
        }
    }

    return GameOverAction::NONE;
}

void GameOverScreen::render(sf::RenderWindow& window)
{
    updateHover(window);

    sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(_windowSize.x),
                                             static_cast<float>(_windowSize.y)));
    overlay.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(overlay);

    window.draw(_titleText);

    for (const auto& text : _scoreTexts) {
        window.draw(text);
    }

    window.draw(_restartText);
    window.draw(_menuText);
    window.draw(_quitText);
}

bool GameOverScreen::isMouseOver(const sf::Text& t, sf::Vector2i mousePos) const
{
    return t.getGlobalBounds().contains(static_cast<float>(mousePos.x),
                                        static_cast<float>(mousePos.y));
}

void GameOverScreen::updateHover(const sf::RenderWindow& window)
{
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);

    _restartText.setFillColor(isMouseOver(_restartText, mousePos) ? _hoverColor : _normalColor);
    _menuText.setFillColor(isMouseOver(_menuText, mousePos) ? _hoverColor : _normalColor);
    _quitText.setFillColor(isMouseOver(_quitText, mousePos) ? _hoverColor : _normalColor);
}

void GameOverScreen::onResize(const sf::Vector2u& size)
{
    _windowSize = size;
    updateLayout();
}