#include "engine/systems/BossHealthBarSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/gameplay/Enemy.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/graphics/Renderer.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <iostream>

BossHealthBarSystem::BossHealthBarSystem() { loadFont(); }

void BossHealthBarSystem::loadFont() {
  if (!_font.loadFromFile("assets/font/star-crush/Star_Crush.ttf")) {
    std::cerr << "[BossHealthBarSystem] Warning: Failed to load font"
              << std::endl;
  }
}

void BossHealthBarSystem::update(Registry &, float) {}

void BossHealthBarSystem::render(Registry &registry, Renderer &renderer) {
  Health *bossHealth = nullptr;
  static int logCounter = 0;

  registry.each<Enemy, Health>([&](EntityID id, Enemy &enemy, Health &h) {
    if (enemy.type == EnemyType::Boss) {
      if (logCounter % 60 == 0) {
        std::cout << "[BossHealthBarSystem] Boss found! HP=" << h.current << "/"
                  << h.max << std::endl;
      }
      bossHealth = &h;
    }
  });

  if (!bossHealth || bossHealth->max <= 0.f) {
    return;
  }

  logCounter++;

  float ratio = bossHealth->current / bossHealth->max;
  if (ratio < 0.f)
    ratio = 0.f;
  if (ratio > 1.f)
    ratio = 1.f;

  auto &window = renderer.getWindow();

  const float LOGICAL_WIDTH = 1280.f;
  const float LOGICAL_HEIGHT = 720.f;

  const float margin = 20.f;
  const float barHeight = 25.f;
  float barWidth = LOGICAL_WIDTH - 2.f * margin;
  float barX = margin;
  float barY = LOGICAL_HEIGHT - barHeight - margin;

  sf::RectangleShape background(sf::Vector2f(barWidth, barHeight));
  background.setFillColor(sf::Color(40, 0, 0, 220));
  background.setPosition(barX, barY);

  sf::RectangleShape foreground(sf::Vector2f(barWidth * ratio, barHeight));
  foreground.setFillColor(sf::Color(220, 40, 40));
  foreground.setPosition(barX, barY);

  window.draw(background);
  window.draw(foreground);

  if (_font.getInfo().family != "") {
    sf::Text text;
    text.setFont(_font);
    text.setString("BOSS1");
    text.setCharacterSize(24);
    text.setFillColor(sf::Color::White);
    text.setOutlineColor(sf::Color::Black);
    text.setOutlineThickness(2.f);

    sf::FloatRect bounds = text.getLocalBounds();
    float textX = barX + (barWidth - bounds.width) * 0.5f - bounds.left;
    float textY = barY - bounds.height - 8.f;
    text.setPosition(textX, textY);

    window.draw(text);
  }
}