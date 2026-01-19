#include "helpers/GameClientHelpers.hpp"
#include "levels/LevelAssetManager.hpp"
#include <SFML/Graphics.hpp>

static LevelAssetManager& getLevelAssetManager() {
    static LevelAssetManager manager;
    return manager;
}

const std::vector<std::string>& getEnemyBasicFrames()
{
    static std::vector<std::string> empty;
    auto* level = getLevelAssetManager().getCurrentLevel();
    return level ? level->getEnemyBasicFrames() : empty;
}

const std::vector<std::string>& getBossFrames()
{
    static std::vector<std::string> empty;
    auto* level = getLevelAssetManager().getCurrentLevel();
    return level ? level->getBossFrames() : empty;
}

const std::vector<std::string>& getBossShotFrames()
{
    static std::vector<std::string> empty;
    auto* level = getLevelAssetManager().getCurrentLevel();
    return level ? level->getBossShotFrames() : empty;
}

void updateMenuView(sf::RenderWindow& window)
{
    sf::Vector2u size = window.getSize();
    sf::View view(sf::FloatRect(0.f, 0.f,
                                static_cast<float>(size.x),
                                static_cast<float>(size.y)));
    window.setView(view);
}

void updateGameView(sf::RenderWindow& window)
{
    constexpr float LOGICAL_WIDTH = 1280.f;
    constexpr float LOGICAL_HEIGHT = 720.f;

    sf::View view(sf::FloatRect(0.f, 0.f, LOGICAL_WIDTH, LOGICAL_HEIGHT));

    sf::Vector2u size = window.getSize();
    float windowRatio = static_cast<float>(size.x) / static_cast<float>(size.y);
    float viewRatio = LOGICAL_WIDTH / LOGICAL_HEIGHT;

    float viewportX = 0.f;
    float viewportY = 0.f;
    float viewportW = 1.f;
    float viewportH = 1.f;

    if (windowRatio > viewRatio) {
        viewportW = viewRatio / windowRatio;
        viewportX = (1.f - viewportW) * 0.5f;
    } else if (windowRatio < viewRatio) {
        viewportH = windowRatio / viewRatio;
        viewportY = (1.f - viewportH) * 0.5f;
    }

    view.setViewport(sf::FloatRect(viewportX, viewportY, viewportW, viewportH));
    window.setView(view);
}