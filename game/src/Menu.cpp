#include "Menu.hpp"
#include "engine/utils/FontManager.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <iostream>

namespace {
static float clampf(float v, float lo, float hi) {
  return std::max(lo, std::min(v, hi));
}
}

Menu::Menu(float windowWidth, float windowHeight, AudioManager &audioManager)
    : _audioManager(audioManager) {
  try {
    FontManager::getInstance().loadFont(
        "assets/font/star-crush/Star_Crush.ttf");
    _font = FontManager::getInstance().getFont();
  } catch (const std::exception &e) {
    std::cerr << "[Menu] FATAL: " << e.what() << std::endl;
    throw;
  }

  if (!_bgTexture.loadFromFile("assets/sprites/rtype_background.jpeg")) {
    std::cerr << "[Menu] Warning: Could not load menu background" << std::endl;
  } else {
    _bgSprite.setTexture(_bgTexture);
    sf::Vector2u texSize = _bgTexture.getSize();
    float scaleX = windowWidth / static_cast<float>(texSize.x);
    float scaleY = windowHeight / static_cast<float>(texSize.y);
    _bgSprite.setScale(scaleX, scaleY);
  }

  if (!_logoTexture.loadFromFile("assets/sprites/R-Type_logo.jpg")) {
    std::cerr << "[Menu] Warning: Could not load logo" << std::endl;
  } else {
    _logoSprite.setTexture(_logoTexture);
    sf::FloatRect bounds = _logoSprite.getLocalBounds();
    _logoSprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
  }

  initButtons(windowWidth, windowHeight);
}

void Menu::initButtons(float windowWidth, float windowHeight) {
  buildMainLayout(windowWidth, windowHeight);
  buildSettingsLayout(windowWidth, windowHeight);
  updateVolumeWidgetLayout(windowWidth, windowHeight);
}

void Menu::updateVolumeWidgetLayout(float windowWidth, float windowHeight) {
  float iconSize = clampf(windowHeight * 0.065f, 28.f, 56.f);
  float margin = clampf(windowWidth * 0.02f, 10.f, 30.f);

  _volume.setSize(iconSize);
  _volume.setPosition({windowWidth - iconSize - margin, margin});
}

bool Menu::isMouseOver(const sf::Text &text, sf::Vector2i mousePos) const {
  return text.getGlobalBounds().contains(static_cast<float>(mousePos.x),
                                         static_cast<float>(mousePos.y));
}

int Menu::getItemCount() const {
  return _inSettings ? 4 : 4;
}

sf::Text& Menu::getItemByIndex(int index) {
  if (!_inSettings) {
    switch (index) {
    case 0:
      return _playText;
    case 1:
      return _soloText;
    case 2:
      return _settingsText;
    default:
      return _quitText;
    }
  }

  switch (index) {
  case 0:
    return _res1024Text;
  case 1:
    return _res1280Text;
  case 2:
    return _res1600Text;
  default:
    return _backText;
  }
}

void Menu::setSelectedIndex(int index) {
  int count = getItemCount();
  if (count <= 0) {
    _selectedIndex = 0;
    return;
  }

  while (index < 0) {
    index += count;
  }
  while (index >= count) {
    index -= count;
  }

  _selectedIndex = index;
}

MenuAction Menu::activateIndex(int index) {
  if (!_inSettings) {
    switch (index) {
    case 0:
      _audioManager.playSound("assets/audio/click_sound.mp3");
      return MenuAction::PLAY;
    case 1:
      _audioManager.playSound("assets/audio/click_sound.mp3");
      return MenuAction::PLAY_ENDLESS;
    case 2:
      _audioManager.playSound("assets/audio/click_sound.mp3");
      _audioManager.playMusic("assets/audio/settings_ost.mp3");
      _inSettings = true;
      setSelectedIndex(0);
      return MenuAction::SETTINGS;
    case 3:
    default:
      _audioManager.playSound("assets/audio/click_sound.mp3");
      return MenuAction::QUIT;
    }
  }

  switch (index) {
  case 0:
    _audioManager.playSound("assets/audio/click_sound.mp3");
    return MenuAction::RES_1024x576;
  case 1:
    _audioManager.playSound("assets/audio/click_sound.mp3");
    return MenuAction::RES_1280x720;
  case 2:
    _audioManager.playSound("assets/audio/click_sound.mp3");
    return MenuAction::RES_1600x900;
  case 3:
  default:
    _audioManager.playSound("assets/audio/click_sound.mp3");
    _audioManager.playMusic("assets/audio/menu_ost.mp3");
    _inSettings = false;
    setSelectedIndex(0);
    return MenuAction::BACK;
  }
}

void Menu::setupButtonText(sf::Text &text, const sf::String &label,
                           unsigned int characterSize, sf::Vector2f position,
                           TextOrigin origin) {
  text.setFont(_font);
  text.setString(label);
  text.setCharacterSize(characterSize);
  text.setFillColor(_normalColor);

  sf::FloatRect b = text.getLocalBounds();

  if (origin == TextOrigin::Center) {
    text.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
  } else {
    text.setOrigin(b.left, b.top + b.height / 2.f);
  }

  text.setPosition(position);
  text.setScale(_normalScale, _normalScale);
}

void Menu::applyVisualState(sf::Vector2i mousePos) {
  int count = getItemCount();

  for (int i = 0; i < count; ++i) {
    sf::Text &t = getItemByIndex(i);
    if (isMouseOver(t, mousePos)) {
      setSelectedIndex(i);
      break;
    }
  }

  for (int i = 0; i < count; ++i) {
    sf::Text &t = getItemByIndex(i);
    bool hovered = isMouseOver(t, mousePos);
    bool selected = (i == _selectedIndex);

    t.setFillColor((hovered || selected) ? _hoverColor : _normalColor);
    float s = selected ? _selectedScale : _normalScale;
    t.setScale(s, s);
  }

  _volume.setMuted(!_audioManager.isMusicEnabled());
  _volume.setColor(_volume.hitTest(mousePos) ? _hoverColor : _normalColor);
}

void Menu::updateHoverAndSelection(const sf::RenderWindow &window) {
  sf::Vector2i mousePos = sf::Mouse::getPosition(window);
  applyVisualState(mousePos);
}

MenuAction Menu::handleEvent(const sf::Event &event) {
  if (event.type == sf::Event::KeyPressed) {
    if (event.key.code == sf::Keyboard::M) {
      _audioManager.toggleMusicEnabled();
      return MenuAction::NONE;
    }

    if (event.key.code == sf::Keyboard::Up) {
      setSelectedIndex(_selectedIndex - 1);
      return MenuAction::NONE;
    }
    if (event.key.code == sf::Keyboard::Down) {
      setSelectedIndex(_selectedIndex + 1);
      return MenuAction::NONE;
    }

    if (event.key.code == sf::Keyboard::Enter ||
        event.key.code == sf::Keyboard::Return) {
      return activateIndex(_selectedIndex);
    }

    if (event.key.code == sf::Keyboard::Escape) {
      if (_inSettings) {
        return activateIndex(3);
      }
      return MenuAction::QUIT;
    }
  }

  if (event.type == sf::Event::MouseButtonPressed &&
      event.mouseButton.button == sf::Mouse::Left) {
    sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);

    if (_volume.hitTest(mousePos)) {
      _audioManager.toggleMusicEnabled();
      _audioManager.playSound("assets/audio/click_sound.mp3");
      return MenuAction::NONE;
    }

    int count = getItemCount();
    for (int i = 0; i < count; ++i) {
      if (isMouseOver(getItemByIndex(i), mousePos)) {
        setSelectedIndex(i);
        return activateIndex(i);
      }
    }
  }

  return MenuAction::NONE;
}

void Menu::render(sf::RenderWindow &window) {
  updateHoverAndSelection(window);

  window.draw(_bgSprite);
  window.draw(_logoSprite);

  if (!_inSettings) {
    window.draw(_playText);
    window.draw(_soloText);
    window.draw(_settingsText);
    window.draw(_quitText);
  } else {
    window.draw(_resolutionLabelText);
    window.draw(_res1024Text);
    window.draw(_res1280Text);
    window.draw(_res1600Text);
    window.draw(_backText);
    window.draw(_controlsTitleText);
    window.draw(_controlsDetailsText);
  }

  _volume.draw(window);
}

void Menu::resize(float windowWidth, float windowHeight) {
  if (_bgTexture.getSize().x > 0) {
    sf::Vector2u texSize = _bgTexture.getSize();
    float scaleX = windowWidth / static_cast<float>(texSize.x);
    float scaleY = windowHeight / static_cast<float>(texSize.y);
    _bgSprite.setScale(scaleX, scaleY);
  }

  initButtons(windowWidth, windowHeight);
}

void Menu::buildMainLayout(float windowWidth, float windowHeight) {
  const float centerY = windowHeight / 2.f;

  const unsigned int fontSize =
      static_cast<unsigned int>(clampf(windowHeight * 0.045f, 22.f, 56.f));
  const float spacing = clampf(windowHeight * 0.085f, 50.f, 90.f);

  float leftX = clampf(windowWidth * 0.18f, 120.f, windowWidth * 0.40f);

  if (_logoTexture.getSize().x > 0) {
    float targetWidth = windowWidth * 0.42f;
    float scale = targetWidth / _logoSprite.getLocalBounds().width;
    _logoSprite.setScale(scale, scale);
    _logoSprite.setPosition(windowWidth / 2.f, windowHeight * 0.20f);
  }

  setupButtonText(_playText, "MULTIJOUEUR", fontSize, {leftX, centerY - spacing * 1.5f},
                  TextOrigin::LeftCenter);
  setupButtonText(_soloText, "SOLO ENDLESS", fontSize, {leftX, centerY - spacing * 0.5f},
                  TextOrigin::LeftCenter);
  setupButtonText(_settingsText, "PARAMETRES", fontSize, {leftX, centerY + spacing * 0.5f},
                  TextOrigin::LeftCenter);
  setupButtonText(_quitText, "QUITTER", fontSize, {leftX, centerY + spacing * 1.5f},
                  TextOrigin::LeftCenter);
}

void Menu::buildSettingsLayout(float windowWidth, float windowHeight) {
  const float centerY = windowHeight / 2.f;

  const unsigned int fontSize =
      static_cast<unsigned int>(clampf(windowHeight * 0.038f, 18.f, 44.f));
  const float spacing = clampf(windowHeight * 0.085f, 50.f, 90.f);

  float leftX = clampf(windowWidth * 0.05f, 60.f, windowWidth * 0.20f);
  float rightX = windowWidth * 0.60f;

  _logoSprite.setPosition(windowWidth / 2.f, windowHeight * 0.17f);

  const float startY = centerY - spacing * 1.0f;

  float margin = clampf(windowWidth * 0.02f, 10.f, 30.f);
  setupButtonText(_backText, "RETOUR", fontSize, {margin, margin},
                  TextOrigin::LeftCenter);
  sf::FloatRect b = _backText.getLocalBounds();
  _backText.setOrigin(b.left, b.top);
  _backText.setPosition(margin, margin);

  setupButtonText(_resolutionLabelText, "RESOLUTION :", fontSize * 1.1f,
                  {leftX, startY - spacing}, TextOrigin::LeftCenter);
  _resolutionLabelText.setFillColor(sf::Color(200, 200, 200));

  setupButtonText(_res1024Text, "1024 x 576", fontSize, {leftX, startY},
                  TextOrigin::LeftCenter);
  setupButtonText(_res1280Text, "1280 x 720", fontSize,
                  {leftX, startY + spacing}, TextOrigin::LeftCenter);
  setupButtonText(_res1600Text, "1600 x 900", fontSize,
                  {leftX, startY + spacing * 2.f}, TextOrigin::LeftCenter);

  setupButtonText(_controlsTitleText, "CONTROLES :", fontSize * 1.1f,
                  {rightX, startY - spacing}, TextOrigin::LeftCenter);
  _controlsTitleText.setFillColor(sf::Color::White);
  _controlsTitleText.setOutlineColor(sf::Color::Black);
  _controlsTitleText.setOutlineThickness(3.0f);

  sf::String controlsStr =
      "Mouvement : Fleches / ZQSD\n\nTirer : Espace\n\nTir Charge : Maintenir "
      "Espace\n\nQuitter : Echap";
  setupButtonText(_controlsDetailsText, controlsStr, fontSize * 0.8f,
                  {rightX, startY + spacing * 0.5f}, TextOrigin::LeftCenter);
  b = _controlsDetailsText.getLocalBounds();
  _controlsDetailsText.setOrigin(b.left, b.top);
  _controlsDetailsText.setPosition(rightX, startY);
  _controlsDetailsText.setFillColor(sf::Color::White);
  _controlsDetailsText.setOutlineColor(sf::Color::Black);
  _controlsDetailsText.setOutlineThickness(3.0f);
}