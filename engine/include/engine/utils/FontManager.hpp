#pragma once

#include <SFML/Graphics/Font.hpp>
#include <string>
#include <memory>
#include <stdexcept>

class FontManager {
public:
    static FontManager& getInstance() {
        static FontManager instance;
        return instance;
    }

    const sf::Font& getFont() const {
        if (!_font) {
            throw std::runtime_error("Font not loaded");
        }
        return *_font;
    }

    void loadFont(const std::string& path) {
        _font = std::make_unique<sf::Font>();
        if (!_font->loadFromFile(path)) {
            throw std::runtime_error("Failed to load required font: " + path);
        }
    }

private:
    FontManager() = default;
    std::unique_ptr<sf::Font> _font;
};