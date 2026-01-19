#include "core/GameManager.hpp"
#include <iostream>
#include <exception>

int main() {
    try {
        GameManager game;
        game.run();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}