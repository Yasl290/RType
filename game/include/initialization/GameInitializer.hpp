#pragma once

#include <engine/core/Registry.hpp>
#include <engine/core/System.hpp>
#include <engine/gameplay/IScoreProvider.hpp>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

class GameInitializer {
public:
    static void initSystems(
        std::vector<std::unique_ptr<System>>& systems,
        float width,
        float height,
        IScoreProvider& scoreProvider,
        bool isSoloMode = false
    );
    
    static void initBackground(
        Registry& registry,
        float windowWidth,
        float windowHeight,
        const std::string& backgroundPath = "assets/sprites/background.png"
    );

    static void updateBackground(
        Registry& registry,
        const std::string& backgroundPath
    );
    
    static void initSoloPlayer(Registry& registry);
    
    static std::string getPlayerTexture(uint8_t slot);
};