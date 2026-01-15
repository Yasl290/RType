#include "engine/graphics/Renderer.hpp"

void Renderer::resetView()
{
    sf::View view(sf::FloatRect(0.f, 0.f,
                                static_cast<float>(_width),
                                static_cast<float>(_height)));
    window->setView(view);
}

Renderer::Renderer(int width, int height, const std::string& title)
    : _width(width), _height(height), _title(title), _isFullscreen(false)
{
    window = std::make_unique<sf::RenderWindow>(
        sf::VideoMode(width, height),
        title,
        sf::Style::Titlebar | sf::Style::Close
    );
    window->setFramerateLimit(60);
    resetView();
}

Renderer::~Renderer()
{
    if (window && window->isOpen()) {
        window->close();
    }
}

void Renderer::clear(sf::Color color)
{
    window->clear(color);
}

void Renderer::display()
{
    window->display();
}

bool Renderer::isOpen() const
{
    return window->isOpen();
}

void Renderer::close()
{
    window->close();
}

bool Renderer::pollEvent(sf::Event& event)
{
    return window->pollEvent(event);
}

sf::RenderWindow& Renderer::getWindow()
{
    return *window;
}

void Renderer::toggleFullscreen()
{
    _isFullscreen = !_isFullscreen;
    
    if (_isFullscreen) {
        window->create(
            sf::VideoMode::getDesktopMode(),
            _title,
            sf::Style::Fullscreen
        );
    } else {
        window->create(
            sf::VideoMode(_width, _height),
            _title,
            sf::Style::Titlebar | sf::Style::Close
        );
    }
    window->setFramerateLimit(60);
    resetView();
}

bool Renderer::isFullscreen() const
{
    return _isFullscreen;
}

void Renderer::setWindowedSize(int width, int height)
{
    _width = width;
    _height = height;
    _isFullscreen = false;
    window->create(
        sf::VideoMode(_width, _height),
        _title,
        sf::Style::Titlebar | sf::Style::Close
    );
    window->setFramerateLimit(60);
    resetView();
}