#pragma once

#include "protocol/Protocol.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class GameOverAction {
    NONE,
    RESTART,
    RETURN_TO_MENU,
    QUIT
};

class GameOverScreen {
public:
    GameOverScreen();

    void setScores(const std::vector<RType::Protocol::PlayerFinalScore>& scores);
    GameOverAction handleEvent(const sf::Event& event);
    void render(sf::RenderWindow& window);
    void onResize(const sf::Vector2u& size);

private:
    sf::Font _font;
    sf::Text _titleText;
    sf::Text _restartText;
    sf::Text _menuText;
    sf::Text _quitText;
    std::vector<sf::Text> _scoreTexts;

    std::vector<RType::Protocol::PlayerFinalScore> _scores;
    int _selectedOption;
    sf::Vector2u _windowSize;

    sf::Color _normalColor = sf::Color::White;
    sf::Color _hoverColor = sf::Color(255, 200, 80);

    void loadFont();
    void updateLayout();
    void sortScoresByRank();
    bool isMouseOver(const sf::Text& t, sf::Vector2i mousePos) const;
    void updateHover(const sf::RenderWindow& window);
};