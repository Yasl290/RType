#include "engine/systems/ScoreDisplaySystem.hpp"
#include "engine/graphics/Renderer.hpp"
#include "engine/utils/FontManager.hpp"
#include <iostream>

ScoreDisplaySystem::ScoreDisplaySystem(IScoreProvider& scoreProvider)
    : _scoreProvider(scoreProvider)
{
    loadFont();
}

void ScoreDisplaySystem::loadFont()
{
    try {
        FontManager::getInstance().loadFont("assets/font/star-crush/Star_Crush.ttf");
        _font = FontManager::getInstance().getFont();
    } catch (const std::exception& e) {
        std::cerr << "[ScoreDisplaySystem] FATAL: " << e.what() << std::endl;
        throw;
    }
}

void ScoreDisplaySystem::update(Registry&, float)
{
}

void ScoreDisplaySystem::render(Registry&, Renderer& renderer)
{
    auto scores = _scoreProvider.getPlayerScores();
    auto& window = renderer.getWindow();
    
    float yOffset = 10.f;
    
    for (const auto& [client_id, scoreData] : scores) {
        sf::Text text;
        text.setFont(_font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(2.f);
        
        std::string scoreText = "Player " + std::to_string(client_id) + 
                                ": " + std::to_string(scoreData.score) + " pts";
        
        if (scoreData.enemies_killed > 0) {
            scoreText += " (" + std::to_string(scoreData.enemies_killed) + " kills)";
        }
        
        text.setString(scoreText);
        text.setPosition(10.f, yOffset);
        
        window.draw(text);
        yOffset += 35.f;
    }
}