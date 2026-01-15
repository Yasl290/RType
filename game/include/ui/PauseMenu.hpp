#pragma once

#include <SFML/Graphics.hpp>
#include <engine/audio/AudioManager.hpp>

#include "../VolumeToggleWidget.hpp"

enum class PauseMenuAction {
    NONE,
    RESUME,
    RETURN_TO_MENU,
    QUIT_APP
};

class PauseMenu {
public:
    explicit PauseMenu(AudioManager& audio);

    void onResize(sf::Vector2u winSize);

    PauseMenuAction handleEvent(const sf::Event& event);

    void render(sf::RenderWindow& window);

private:
    static float clampf(float v, float lo, float hi);

    bool isMouseOver(const sf::Text& t, sf::Vector2i mousePos) const;
    void updateHover(const sf::RenderWindow& window);

    void setupText(sf::Text& t, const sf::String& label, unsigned int size, sf::Vector2f pos);

    AudioManager& _audio;

    sf::Font _font;

    sf::Text _resumeText;
    sf::Text _backToMenuText;
    sf::Text _quitText;

    sf::Texture _bgTexture;
    sf::Sprite _bgSprite;
    bool _bgReady = false;

    sf::RectangleShape _fallbackDim;

    sf::Color _normalColor = sf::Color::White;
    sf::Color _hoverColor = sf::Color(255, 200, 80);

    VolumeToggleWidget _volume;
};
