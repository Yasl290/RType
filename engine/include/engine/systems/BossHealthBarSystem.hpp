#pragma once

#include "engine/core/System.hpp"
#include <SFML/Graphics/Font.hpp>

class BossHealthBarSystem : public System {
public:
    BossHealthBarSystem();
    ~BossHealthBarSystem() override = default;

    void update(Registry& registry, float deltaTime) override;
    void render(Registry& registry, Renderer& renderer) override;

private:
    sf::Font _font;
    void loadFont();
};
