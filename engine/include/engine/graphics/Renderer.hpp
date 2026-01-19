#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

class Renderer {
public:
    Renderer(int width, int height, const std::string& title);
    ~Renderer();

    void clear(sf::Color color);
    void display();

    bool isOpen() const;
    void close();

    bool pollEvent(sf::Event& event);

    sf::RenderWindow& getWindow();

    void toggleFullscreen();
    bool isFullscreen() const;
    void setWindowedSize(int width, int height);

private:
    void resetView();

    std::unique_ptr<sf::RenderWindow> window;
    int _width;
    int _height;
    std::string _title;
    bool _isFullscreen;
};