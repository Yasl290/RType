#include "ui/UpgradeMenu.hpp"
#include <iostream>

UpgradeMenu::UpgradeMenu(AudioManager& audio)
    : _audio(audio)
    , _visible(false)
    , _selectedIndex(0)
    , _windowSize(1280, 720)
{
    if (!_font.loadFromFile("assets/font/star-crush/Star_Crush.ttf")) {
        std::cerr << "[UpgradeMenu] Failed to load font" << std::endl;
    }
    
    _background.setFillColor(sf::Color(0, 0, 0, 200));
    
    _title.setFont(_font);
    _title.setString("CHOOSE YOUR UPGRADE");
    _title.setCharacterSize(48);
    _title.setFillColor(sf::Color(255, 215, 0));
    _title.setOutlineColor(sf::Color::Black);
    _title.setOutlineThickness(2);
}

void UpgradeMenu::show(const std::vector<UpgradeType>& choices) {
    _visible = true;
    _choices = choices;
    _selectedIndex = 0;
    

    _cards.clear();
    for (size_t i = 0; i < choices.size(); ++i) {
        const auto* def = UpgradeDatabase::getUpgrade(choices[i]);
        if (!def) continue;
        
        UpgradeCard card;
        card.isHovered = (i == 0);
        
        card.background.setFillColor(sf::Color(30, 30, 60, 230));
        card.border.setFillColor(sf::Color::Transparent);
        card.border.setOutlineThickness(4);
        card.border.setOutlineColor(card.isHovered ? sf::Color(255, 215, 0) : sf::Color(100, 100, 150));
        
        card.name.setFont(_font);
        card.name.setString(def->name);
        card.name.setCharacterSize(32);
        card.name.setFillColor(sf::Color::White);
        
        card.description.setFont(_font);
        card.description.setString(def->description);
        card.description.setCharacterSize(20);
        card.description.setFillColor(sf::Color(200, 200, 200));
        
        card.level.setFont(_font);
        card.level.setCharacterSize(18);
        card.level.setFillColor(sf::Color(150, 150, 150));
        
        _cards.push_back(card);
    }
    
    updateLayout();
    
    std::cout << "[UpgradeMenu] Showing " << choices.size() << " upgrade choices" << std::endl;
}

void UpgradeMenu::hide() {
    _visible = false;
    _choices.clear();
    _cards.clear();
}

void UpgradeMenu::handleEvent(const sf::Event& event) {
    if (!_visible) return;
    
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
            _selectedIndex = (_selectedIndex - 1 + _cards.size()) % _cards.size();
            _audio.playSound("assets/audio/click_sound.mp3");
            updateLayout();
        }
        else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
            _selectedIndex = (_selectedIndex + 1) % _cards.size();
            _audio.playSound("assets/audio/click_sound.mp3");
            updateLayout();
        }
        else if (event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::Enter) {
            selectUpgrade(_selectedIndex);
        }
        else if (event.key.code == sf::Keyboard::Num1 && _cards.size() >= 1) {
            selectUpgrade(0);
        }
        else if (event.key.code == sf::Keyboard::Num2 && _cards.size() >= 2) {
            selectUpgrade(1);
        }
        else if (event.key.code == sf::Keyboard::Num3 && _cards.size() >= 3) {
            selectUpgrade(2);
        }
    }
    
    if (event.type == sf::Event::MouseMoved) {
        for (size_t i = 0; i < _cards.size(); ++i) {
            if (_cards[i].background.getGlobalBounds().contains(event.mouseMove.x, event.mouseMove.y)) {
                if (_selectedIndex != static_cast<int>(i)) {
                    _selectedIndex = i;
                    updateLayout();
                }
                break;
            }
        }
    }
    
    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            for (size_t i = 0; i < _cards.size(); ++i) {
                if (_cards[i].background.getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y)) {
                    selectUpgrade(i);
                    break;
                }
            }
        }
    }
}

void UpgradeMenu::selectUpgrade(int index) {
    if (index < 0 || index >= static_cast<int>(_choices.size())) return;
    
    _audio.playSound("assets/audio/click_sound.mp3");
    
    if (_onUpgradeSelected) {
        _onUpgradeSelected(_choices[index]);
    }
    
    hide();
}

void UpgradeMenu::updateLayout() {
    float width = static_cast<float>(_windowSize.x);
    float height = static_cast<float>(_windowSize.y);
    
    _background.setSize(sf::Vector2f(width, height));
    

    sf::FloatRect titleBounds = _title.getLocalBounds();
    _title.setPosition((width - titleBounds.width) / 2.0f, height * 0.15f);
    
    float cardWidth = 300.0f;
    float cardHeight = 200.0f;
    float spacing = 30.0f;
    float totalWidth = _cards.size() * cardWidth + (_cards.size() - 1) * spacing;
    float startX = (width - totalWidth) / 2.0f;
    float cardY = height * 0.35f;
    
    for (size_t i = 0; i < _cards.size(); ++i) {
        float cardX = startX + i * (cardWidth + spacing);
        
        _cards[i].background.setSize(sf::Vector2f(cardWidth, cardHeight));
        _cards[i].background.setPosition(cardX, cardY);
        
        _cards[i].border.setSize(sf::Vector2f(cardWidth, cardHeight));
        _cards[i].border.setPosition(cardX, cardY);
        _cards[i].border.setOutlineColor(_selectedIndex == static_cast<int>(i) ? 
            sf::Color(255, 215, 0) : sf::Color(100, 100, 150));
        
        _cards[i].name.setPosition(cardX + 10, cardY + 20);
        _cards[i].description.setPosition(cardX + 10, cardY + 70);
        _cards[i].level.setPosition(cardX + 10, cardY + cardHeight - 40);
    }
}

void UpgradeMenu::render(sf::RenderWindow& window) {
    if (!_visible) return;
    
    window.draw(_background);
    window.draw(_title);
    
    for (auto& card : _cards) {
        window.draw(card.background);
        window.draw(card.border);
        window.draw(card.name);
        window.draw(card.description);
        window.draw(card.level);
    }
    
    sf::Text hint;
    hint.setFont(_font);
    hint.setString("Arrow Keys / Mouse to select  |  Space / Click to confirm  |  1/2/3 for quick select");
    hint.setCharacterSize(18);
    hint.setFillColor(sf::Color(150, 150, 150));
    sf::FloatRect hintBounds = hint.getLocalBounds();
    hint.setPosition((_windowSize.x - hintBounds.width) / 2.0f, _windowSize.y * 0.85f);
    window.draw(hint);
}

void UpgradeMenu::onResize(const sf::Vector2u& windowSize) {
    _windowSize = windowSize;
    updateLayout();
}