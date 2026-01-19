#pragma once

#include <SFML/Graphics.hpp>
#include <engine/audio/AudioManager.hpp>

#include "../VolumeToggleWidget.hpp"

class NetworkClient;
class CheatManager;

enum class PauseMenuAction {
    NONE,
    RESUME,
    RETURN_TO_MENU,
    QUIT_APP,
    TOGGLE_DOUBLE_DAMAGE,
    TOGGLE_INVINCIBILITY,
    TOGGLE_DOUBLE_FIRE_RATE
};

class PauseMenu {
public:
    explicit PauseMenu(AudioManager& audio, NetworkClient* network = nullptr, CheatManager* cheats = nullptr);

    void onResize(sf::Vector2u winSize);

    PauseMenuAction handleEvent(const sf::Event& event);

    void render(sf::RenderWindow& window);

private:
    static float clampf(float v, float lo, float hi);

    bool isMouseOver(const sf::Text& t, sf::Vector2i mousePos) const;
    void updateHover(const sf::RenderWindow& window);

    void setupText(sf::Text& t, const sf::String& label, unsigned int size, sf::Vector2f pos);
    bool isSoloPlayer() const;
    sf::String getCheatButtonText(const sf::String& label, bool active) const;

    AudioManager& _audio;
    NetworkClient* _network;
    CheatManager* _cheats;

    sf::Font _font;

    sf::Text _resumeText;
    sf::Text _backToMenuText;
    sf::Text _quitText;

    sf::Text _cheatTitle;
    sf::Text _doubleDamageText;
    sf::Text _invincibilityText;
    sf::Text _doubleFireRateText;

    sf::Texture _bgTexture;
    sf::Sprite _bgSprite;
    bool _bgReady = false;

    sf::RectangleShape _fallbackDim;

    sf::Color _normalColor = sf::Color::White;
    sf::Color _hoverColor = sf::Color(255, 200, 80);
    sf::Color _cheatOnColor = sf::Color(100, 255, 100);
    sf::Color _cheatOffColor = sf::Color(200, 200, 200);

    VolumeToggleWidget _volume;
};
