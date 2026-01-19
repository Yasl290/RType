#include "core/MenuLoop.hpp"
#include <iostream>

MenuLoop::MenuLoop(Renderer& renderer, Menu& menu)
    : _renderer(renderer), _menu(menu)
{
}

int MenuLoop::run() {
    sf::Vector2u size = _renderer.getWindow().getSize();
    sf::View view(sf::FloatRect(0.f, 0.f,
                                static_cast<float>(size.x),
                                static_cast<float>(size.y)));
    _renderer.getWindow().setView(view);

    while (_renderer.isOpen()) {
        sf::Event event;
        while (_renderer.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                _renderer.close();
                return 0;
            }

            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::F11) {
                _renderer.toggleFullscreen();
                handleResize();
            }

            MenuAction action = _menu.handleEvent(event);
            switch (action) {
                case MenuAction::PLAY:
                    std::cout << ">> PLAY clicked - Ready to play!" << std::endl;
                    return 1;
                
                case MenuAction::PLAY_ENDLESS:
                    std::cout << ">> ENDLESS MODE clicked - Ready to play solo!" << std::endl;
                    return 2;
                    
                case MenuAction::QUIT:
                    std::cout << ">> QUIT clicked" << std::endl;
                    _renderer.close();
                    return 0;
                    
                case MenuAction::RES_1024x576:
                    _renderer.setWindowedSize(1024, 576);
                    handleResize();
                    _menu.resize(1024.f, 576.f);
                    break;
                    
                case MenuAction::RES_1280x720:
                    _renderer.setWindowedSize(1280, 720);
                    handleResize();
                    _menu.resize(1280.f, 720.f);
                    break;
                    
                case MenuAction::RES_1600x900:
                    _renderer.setWindowedSize(1600, 900);
                    handleResize();
                    _menu.resize(1600.f, 900.f);
                    break;
                    
                case MenuAction::TOGGLE_FULLSCREEN:
                    _renderer.toggleFullscreen();
                    handleResize();
                    break;
                    
                case MenuAction::SETTINGS:
                case MenuAction::BACK:
                case MenuAction::NONE:
                default:
                    break;
            }
        }

        _renderer.clear(sf::Color(20, 20, 30));
        _menu.render(_renderer.getWindow());
        _renderer.display();
    }
    
    return 0;
}

void MenuLoop::handleResize() {
    sf::Vector2u size = _renderer.getWindow().getSize();
    sf::View view(sf::FloatRect(0.f, 0.f, static_cast<float>(size.x),
                                static_cast<float>(size.y)));
    _renderer.getWindow().setView(view);
    _menu.resize(static_cast<float>(size.x), static_cast<float>(size.y));
}