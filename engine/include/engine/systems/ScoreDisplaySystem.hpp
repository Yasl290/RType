#pragma once

#include "engine/core/System.hpp"
#include "engine/gameplay/IScoreProvider.hpp"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>

class ScoreDisplaySystem : public System {
public:
    explicit ScoreDisplaySystem(IScoreProvider& scoreProvider);
    ~ScoreDisplaySystem() override = default;
    
    void update(Registry& registry, float deltaTime) override;
    void render(Registry& registry, Renderer& renderer) override;

private:
    IScoreProvider& _scoreProvider;
    sf::Font _font;
    
    void loadFont();
};