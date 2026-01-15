#pragma once

#include <SFML/Graphics.hpp>
#include <engine/audio/AudioManager.hpp>

#include <vector>

#include "../../include/NetworkClient.hpp"
#include "../VolumeToggleWidget.hpp"

enum class LobbyScreenResult {
    START_GAME,
    RETURN_TO_MENU,
    QUIT_APP
};

class LobbyScreen {
public:
    LobbyScreen(NetworkClient& network, AudioManager& audio);

    LobbyScreenResult run(class Renderer& renderer);

private:
    static float clampf(float v, float lo, float hi);

    void onResize(sf::Vector2u winSize);

    void setupText(sf::Text& t, const sf::String& label, unsigned int size, sf::Vector2f pos);
    bool isMouseOver(const sf::Text& t, sf::Vector2i mousePos) const;
    void updateHover(const sf::RenderWindow& window);

    void updateFromNetwork();

    NetworkClient& _network;
    AudioManager& _audio;

    bool _localReady = false;

    sf::Font _font;

    sf::Texture _bgTexture;
    sf::Sprite _bgSprite;
    bool _bgReady = false;

    sf::RectangleShape _fallbackDim;

    sf::Text _titleText;
    sf::Text _infoText;
    sf::Text _readyText;
    sf::Text _backText;

    std::vector<sf::Text> _slotTexts;

    sf::Color _normalColor = sf::Color::White;
    sf::Color _hoverColor = sf::Color(255, 200, 80);

    VolumeToggleWidget _volume;
};
