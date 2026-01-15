#pragma once

#include <SFML/Graphics.hpp>
#include <engine/audio/AudioManager.hpp>

#include "VolumeToggleWidget.hpp"

enum class MenuAction {
    NONE,
    PLAY,
    QUIT,
    SETTINGS,
    BACK,
    RES_1024x576,
    RES_1280x720,
    RES_1600x900,
    TOGGLE_FULLSCREEN
};

class Menu {
public:
    Menu(float windowWidth, float windowHeight, AudioManager& audioManager);

    MenuAction handleEvent(const sf::Event& event);
    void render(sf::RenderWindow& window);
    void resize(float windowWidth, float windowHeight);

private:
    enum class TextOrigin {
        Center,
        LeftCenter
    };

    void initButtons(float windowWidth, float windowHeight);

    bool isMouseOver(const sf::Text& text, sf::Vector2i mousePos) const;

    int getItemCount() const;
    sf::Text& getItemByIndex(int index);
    MenuAction activateIndex(int index);

    void setSelectedIndex(int index);
    void updateHoverAndSelection(const sf::RenderWindow& window);
    void applyVisualState(sf::Vector2i mousePos);

    void buildMainLayout(float windowWidth, float windowHeight);
    void buildSettingsLayout(float windowWidth, float windowHeight);

    void setupButtonText(sf::Text& text,
        const sf::String& label,
        unsigned int characterSize,
        sf::Vector2f position,
        TextOrigin origin);

    void updateVolumeWidgetLayout(float windowWidth, float windowHeight);

    sf::Font _font;
    bool _inSettings = false;

    sf::Color _normalColor = sf::Color::White;
    sf::Color _hoverColor = sf::Color(255, 200, 80);

    int _selectedIndex = 0;
    float _selectedScale = 1.08f;
    float _normalScale = 1.0f;

    sf::Text _playText;
    sf::Text _settingsText;
    sf::Text _quitText;

    sf::Text _res1024Text;
    sf::Text _res1280Text;
    sf::Text _res1600Text;
    sf::Text _fullscreenText;
    sf::Text _backText;

    sf::Texture _logoTexture;
    sf::Sprite _logoSprite;

    AudioManager& _audioManager;

    sf::Texture _bgTexture;
    sf::Sprite _bgSprite;

    VolumeToggleWidget _volume;
};
