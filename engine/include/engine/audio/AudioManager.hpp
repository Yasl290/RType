#pragma once

#include <SFML/Audio.hpp>
#include <list>
#include <map>
#include <string>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void playMusic(const std::string& filePath, bool loop = true);
    void stopMusic();

    void setMusicEnabled(bool enabled);
    void toggleMusicEnabled();
    bool isMusicEnabled() const;

    void playSound(const std::string& filePath);

private:
    void restartLastMusicIfPossible();

    sf::Music _music;
    bool _musicEnabled = true;
    std::string _lastMusicPath;
    bool _lastMusicLoop = true;

    std::map<std::string, sf::SoundBuffer> _soundBuffers;
    std::list<sf::Sound> _sounds;
};
