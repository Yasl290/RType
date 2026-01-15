#include "ui/PauseMenu.hpp"

#include <iostream>

PauseMenu::PauseMenu(AudioManager &audio) : _audio(audio) {
  if (!_font.loadFromFile("assets/font/star-crush/Star_Crush.ttf") &&
      !_font.loadFromFile("assets/font/star-crush/Star_Crush.otf") &&
      !_font.loadFromFile("/System/Library/Fonts/Helvetica.ttc") &&
      !_font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf") &&
      !_font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
    std::cerr << "Warning: Could not load font (PauseMenu)" << std::endl;
  }

  _bgReady = _bgTexture.loadFromFile("assets/sprites/pause_background.jpeg");
  if (_bgReady) {
    _bgSprite.setTexture(_bgTexture);
  }
}

float PauseMenu::clampf(float v, float lo, float hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

void PauseMenu::setupText(sf::Text &t, const sf::String &label,
                          unsigned int size, sf::Vector2f pos) {
  t.setFont(_font);
  t.setString(label);
  t.setCharacterSize(size);
  t.setFillColor(_normalColor);
  sf::FloatRect b = t.getLocalBounds();
  t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
  t.setPosition(pos);
}

void PauseMenu::onResize(sf::Vector2u winSize) {
  float w = static_cast<float>(winSize.x);
  float h = static_cast<float>(winSize.y);

  unsigned int titleSize =
      static_cast<unsigned int>(clampf(h * 0.055f, 22.f, 60.f));
  unsigned int itemSize =
      static_cast<unsigned int>(clampf(h * 0.042f, 18.f, 46.f));

  float centerX = w / 2.f;
  float centerY = h / 2.f;
  float spacing = clampf(h * 0.10f, 55.f, 110.f);

  setupText(_resumeText, "REPRENDRE", titleSize, {centerX, centerY - spacing});
  setupText(_backToMenuText, "RETOUR AU MENU", itemSize, {centerX, centerY});
  setupText(_quitText, "QUITTER LE JEU", itemSize,
            {centerX, centerY + spacing});

  if (_bgReady && _bgTexture.getSize().x > 0) {
    sf::Vector2u ts = _bgTexture.getSize();
    float sx = w / static_cast<float>(ts.x);
    float sy = h / static_cast<float>(ts.y);
    _bgSprite.setScale(sx, sy);
    _bgSprite.setPosition(0.f, 0.f);
  }

  _fallbackDim.setSize({w, h});
  _fallbackDim.setFillColor(sf::Color(0, 0, 0, 180));

  float iconSize = clampf(h * 0.065f, 28.f, 56.f);
  float margin = clampf(w * 0.02f, 10.f, 30.f);
  _volume.setSize(iconSize);
  _volume.setPosition({w - iconSize - margin, margin});
}

bool PauseMenu::isMouseOver(const sf::Text &t, sf::Vector2i mousePos) const {
  return t.getGlobalBounds().contains(static_cast<float>(mousePos.x),
                                      static_cast<float>(mousePos.y));
}

void PauseMenu::updateHover(const sf::RenderWindow &window) {
  sf::Vector2i mousePos = sf::Mouse::getPosition(window);

  auto setHover = [&](sf::Text &t) {
    t.setFillColor(isMouseOver(t, mousePos) ? _hoverColor : _normalColor);
  };

  setHover(_resumeText);
  setHover(_backToMenuText);
  setHover(_quitText);

  _volume.setMuted(!_audio.isMusicEnabled());
  _volume.setColor(_volume.hitTest(mousePos) ? _hoverColor : _normalColor);
}

PauseMenuAction PauseMenu::handleEvent(const sf::Event &event) {
  if (event.type == sf::Event::KeyPressed &&
      event.key.code == sf::Keyboard::M) {
    _audio.toggleMusicEnabled();
    return PauseMenuAction::NONE;
  }

  if (event.type == sf::Event::MouseButtonPressed &&
      event.mouseButton.button == sf::Mouse::Left) {
    sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);

    if (_volume.hitTest(mousePos)) {
      _audio.toggleMusicEnabled();
      _audio.playSound("assets/audio/click_sound.mp3");
      return PauseMenuAction::NONE;
    }

    if (isMouseOver(_resumeText, mousePos)) {
      return PauseMenuAction::RESUME;
    }
    if (isMouseOver(_backToMenuText, mousePos)) {
      return PauseMenuAction::RETURN_TO_MENU;
    }
    if (isMouseOver(_quitText, mousePos)) {
      return PauseMenuAction::QUIT_APP;
    }
  }

  return PauseMenuAction::NONE;
}

void PauseMenu::render(sf::RenderWindow &window) {
  updateHover(window);

  if (_bgReady) {
    window.draw(_bgSprite);
  } else {
    window.draw(_fallbackDim);
  }

  window.draw(_resumeText);
  window.draw(_backToMenuText);
  window.draw(_quitText);
  _volume.draw(window);
}
