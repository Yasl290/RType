#pragma once

#include <engine/graphics/Renderer.hpp>
#include "Menu.hpp"

class MenuLoop {
public:
    MenuLoop(Renderer& renderer, Menu& menu);
    
    int run();

private:
    void handleResize();
    
    Renderer& _renderer;
    Menu& _menu;
};