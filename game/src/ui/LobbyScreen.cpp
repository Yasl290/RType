#include "ui/LobbyScreen.hpp"

#include <engine/graphics/Renderer.hpp>
#include <iostream>

namespace {
static void recenterText(sf::Text &t) {
  sf::FloatRect b = t.getLocalBounds();
  t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
}
} // namespace

LobbyScreen::LobbyScreen(NetworkClient &network, AudioManager &audio)
    : _network(network), _audio(audio) {
  if (!_font.loadFromFile("assets/font/star-crush/Star_Crush.ttf") &&
      !_font.loadFromFile("assets/font/star-crush/Star_Crush.otf") &&
      !_font.loadFromFile("/System/Library/Fonts/Helvetica.ttc") &&
      !_font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf") &&
      !_font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
    std::cerr << "Warning: Could not load font (LobbyScreen)" << std::endl;
  }

  _bgReady = _bgTexture.loadFromFile("assets/sprites/pause_background.jpeg");
  if (_bgReady) {
    _bgSprite.setTexture(_bgTexture);
  }

  _slotTexts.resize(4);
}

float LobbyScreen::clampf(float v, float lo, float hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

void LobbyScreen::setupText(sf::Text &t, const sf::String &label,
                            unsigned int size, sf::Vector2f pos) {
  t.setFont(_font);
  t.setString(label);
  t.setCharacterSize(size);
  t.setFillColor(_normalColor);
  recenterText(t);
  t.setPosition(pos);
}

bool LobbyScreen::isMouseOver(const sf::Text &t, sf::Vector2i mousePos) const {
  return t.getGlobalBounds().contains(static_cast<float>(mousePos.x),
                                      static_cast<float>(mousePos.y));
}

void LobbyScreen::onResize(sf::Vector2u winSize) {
  float w = static_cast<float>(winSize.x);
  float h = static_cast<float>(winSize.y);

  _fallbackDim.setSize({w, h});
  _fallbackDim.setFillColor(sf::Color(0, 0, 0, 180));

  if (_bgReady && _bgTexture.getSize().x > 0) {
    sf::Vector2u ts = _bgTexture.getSize();
    float sx = w / static_cast<float>(ts.x);
    float sy = h / static_cast<float>(ts.y);
    _bgSprite.setScale(sx, sy);
    _bgSprite.setPosition(0.f, 0.f);
  }

  unsigned int titleSize =
      static_cast<unsigned int>(clampf(h * 0.055f, 22.f, 60.f));
  unsigned int infoSize =
      static_cast<unsigned int>(clampf(h * 0.032f, 16.f, 34.f));
  unsigned int itemSize =
      static_cast<unsigned int>(clampf(h * 0.040f, 18.f, 44.f));

  float centerX = w / 2.f;
  float topY = h * 0.18f;

  float startY = h * 0.40f;
  float line = clampf(h * 0.065f, 34.f, 60.f);

  setupText(_titleText, "LOBBY", titleSize, {centerX, topY});
  setupText(_infoText, "En attente du statut lobby...", infoSize,
            {centerX, topY + line});

  setupText(_readyText, "PRET: NON  (ENTREE / ESPACE)", itemSize,
            {centerX, startY});

  for (size_t i = 0; i < _slotTexts.size(); ++i) {
    setupText(_slotTexts[i], "Joueur", infoSize,
              {centerX, startY + line * (1.f + static_cast<float>(i))});
  }

  setupText(
      _backText, "RETOUR (ECHAP)", infoSize,
      {centerX,
       startY + line * (1.f + static_cast<float>(_slotTexts.size()) + 1.f)});

  float iconSize = clampf(h * 0.065f, 28.f, 56.f);
  float margin = clampf(w * 0.02f, 10.f, 30.f);
  _volume.setSize(iconSize);
  _volume.setPosition({w - iconSize - margin, margin});
}

void LobbyScreen::updateFromNetwork() {
  if (_network.isGameOn()) {
    _network.clearGameOn();
  }

  if (!_network.hasLobbyStatus()) {
    _infoText.setString("En attente du statut lobby...");
    recenterText(_infoText);
    return;
  }

  LobbyStatusSnapshot snap = _network.getLobbyStatus();

  _infoText.setString(
      "Joueurs: " + std::to_string(static_cast<int>(snap.players_connected)) +
      "/" + std::to_string(static_cast<int>(snap.max_players)) +
      " | Prets: " + std::to_string(static_cast<int>(snap.players_ready)));
  recenterText(_infoText);

  bool meReady = (snap.ready_mask &
                  static_cast<uint8_t>(1u << _network.getPlayerSlot())) != 0;
  _localReady = meReady;

  _readyText.setString(std::string("PRET: ") + (_localReady ? "OUI" : "NON") +
                       "  (ENTREE / ESPACE)");
  recenterText(_readyText);

  for (size_t i = 0; i < _slotTexts.size(); ++i) {
    bool ready = (snap.ready_mask & static_cast<uint8_t>(1u << i)) != 0;
    std::string label = "Joueur " + std::to_string(static_cast<int>(i + 1)) +
                        ": " + (ready ? "PRET" : "PAS PRET");
    if (i == _network.getPlayerSlot()) {
      label += "  (VOUS)";
    }
    _slotTexts[i].setString(label);
    recenterText(_slotTexts[i]);
  }
}

void LobbyScreen::updateHover(const sf::RenderWindow &window) {
  sf::Vector2i mousePos = sf::Mouse::getPosition(window);

  _readyText.setFillColor(isMouseOver(_readyText, mousePos) ? _hoverColor
                                                            : _normalColor);
  _backText.setFillColor(isMouseOver(_backText, mousePos) ? _hoverColor
                                                          : _normalColor);

  _volume.setMuted(!_audio.isMusicEnabled());
  _volume.setColor(_volume.hitTest(mousePos) ? _hoverColor : _normalColor);
}

LobbyScreenResult LobbyScreen::run(Renderer &renderer) {
  onResize(renderer.getWindow().getSize());

  while (renderer.isOpen()) {
    updateFromNetwork();

    if (_network.isGameOn()) {
      _network.clearGameOn();
      return LobbyScreenResult::START_GAME;
    }
    if (_network.hasLobbyStatus() && _network.getLobbyStatus().game_started) {
      return LobbyScreenResult::START_GAME;
    }

    sf::Event event;
    while (renderer.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        renderer.close();
        return LobbyScreenResult::QUIT_APP;
      }

      if (event.type == sf::Event::Resized) {
        sf::Vector2u size = renderer.getWindow().getSize();
        sf::View view(sf::FloatRect(0.f, 0.f, static_cast<float>(size.x),
                                    static_cast<float>(size.y)));
        renderer.getWindow().setView(view);
        onResize(size);
      }

      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::F11) {
        renderer.toggleFullscreen();
        sf::Vector2u size = renderer.getWindow().getSize();
        sf::View view(sf::FloatRect(0.f, 0.f, static_cast<float>(size.x),
                                    static_cast<float>(size.y)));
        renderer.getWindow().setView(view);
        onResize(size);
        continue;
      }

      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::M) {
        _audio.toggleMusicEnabled();
        continue;
      }

      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::Escape) {
        _network.sendReady(false);
        return LobbyScreenResult::RETURN_TO_MENU;
      }

      if (event.type == sf::Event::KeyPressed &&
          (event.key.code == sf::Keyboard::Enter ||
           event.key.code == sf::Keyboard::Return ||
           event.key.code == sf::Keyboard::Space)) {
        _localReady = !_localReady;
        _network.sendReady(_localReady);
        _audio.playSound("assets/audio/click_sound.mp3");
        continue;
      }

      if (event.type == sf::Event::MouseButtonPressed &&
          event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i mousePos(event.mouseButton.x, event.mouseButton.y);

        if (_volume.hitTest(mousePos)) {
          _audio.toggleMusicEnabled();
          _audio.playSound("assets/audio/click_sound.mp3");
          continue;
        }

        if (isMouseOver(_readyText, mousePos)) {
          _localReady = !_localReady;
          _network.sendReady(_localReady);
          _audio.playSound("assets/audio/click_sound.mp3");
          continue;
        }

        if (isMouseOver(_backText, mousePos)) {
          _network.sendReady(false);
          return LobbyScreenResult::RETURN_TO_MENU;
        }
      }
    }

    renderer.clear(sf::Color(0, 0, 0));

    updateHover(renderer.getWindow());

    if (_bgReady) {
      renderer.getWindow().draw(_bgSprite);
    } else {
      renderer.getWindow().draw(_fallbackDim);
    }

    renderer.getWindow().draw(_titleText);
    renderer.getWindow().draw(_infoText);
    renderer.getWindow().draw(_readyText);

    for (auto &t : _slotTexts) {
      renderer.getWindow().draw(t);
    }

    renderer.getWindow().draw(_backText);
    _volume.draw(renderer.getWindow());

    renderer.display();
  }

  return LobbyScreenResult::QUIT_APP;
}
