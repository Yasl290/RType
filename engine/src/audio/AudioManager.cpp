#include "engine/audio/AudioManager.hpp"
#include <iostream>

AudioManager::AudioManager() {}

AudioManager::~AudioManager()
{
    stopMusic();
    _sounds.clear();
}

bool AudioManager::isMusicEnabled() const
{
    return _musicEnabled;
}

void AudioManager::restartLastMusicIfPossible()
{
    if (_lastMusicPath.empty()) {
        return;
    }

    if (_music.openFromFile(_lastMusicPath)) {
        _music.setLoop(_lastMusicLoop);
        _music.play();
    } else {
        std::cerr << "[AudioManager] Failed to load music: " << _lastMusicPath << std::endl;
    }
}

void AudioManager::setMusicEnabled(bool enabled)
{
    if (_musicEnabled == enabled) {
        return;
    }

    _musicEnabled = enabled;

    if (_musicEnabled) {
        restartLastMusicIfPossible();
    } else {
        _music.stop();
    }
}

void AudioManager::toggleMusicEnabled()
{
    setMusicEnabled(!_musicEnabled);
}

void AudioManager::playMusic(const std::string& filePath, bool loop)
{
    _lastMusicPath = filePath;
    _lastMusicLoop = loop;

    if (!_musicEnabled) {
        _music.stop();
        return;
    }

    if (_music.openFromFile(filePath)) {
        _music.setLoop(loop);
        _music.play();
    } else {
        std::cerr << "[AudioManager] Failed to load music: " << filePath << std::endl;
    }
}

void AudioManager::stopMusic()
{
    _music.stop();
}

void AudioManager::playSound(const std::string& filePath)
{
    if (_soundBuffers.find(filePath) == _soundBuffers.end()) {
        sf::SoundBuffer buffer;
        if (!buffer.loadFromFile(filePath)) {
            std::cerr << "[AudioManager] Failed to load sound: " << filePath << std::endl;
            return;
        }
        _soundBuffers[filePath] = buffer;
    }
    _sounds.remove_if([](const sf::Sound& s) {
        return s.getStatus() == sf::Sound::Stopped;
    });

    _sounds.emplace_back();
    sf::Sound& sound = _sounds.back();
    sound.setBuffer(_soundBuffers[filePath]);
    sound.play();
}
