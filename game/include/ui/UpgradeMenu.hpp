#pragma once

#include <SFML/Graphics.hpp>
#include <engine/audio/AudioManager.hpp>
#include <engine/gameplay/UpgradeDefinitions.hpp>
#include <vector>
#include <functional>

class UpgradeMenu {
public:
    UpgradeMenu(AudioManager& audio);
    
    void show(const std::vector<UpgradeType>& choices);
    void hide();
    bool isVisible() const { return _visible; }
    
    void handleEvent(const sf::Event& event);
    void render(sf::RenderWindow& window);
    void onResize(const sf::Vector2u& windowSize);
    

    void setOnUpgradeSelected(std::function<void(UpgradeType)> callback) {
        _onUpgradeSelected = callback;
    }

private:
    void selectUpgrade(int index);
    void updateLayout();
    
    AudioManager& _audio;
    bool _visible;
    
    std::vector<UpgradeType> _choices;
    int _selectedIndex;
    
    sf::Font _font;
    sf::RectangleShape _background;
    sf::Text _title;
    
    struct UpgradeCard {
        sf::RectangleShape background;
        sf::RectangleShape border;
        sf::Text name;
        sf::Text description;
        sf::Text level;
        bool isHovered;
    };
    std::vector<UpgradeCard> _cards;
    
    sf::Vector2u _windowSize;
    
    std::function<void(UpgradeType)> _onUpgradeSelected;
};