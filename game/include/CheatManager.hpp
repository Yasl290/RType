#pragma once

#include <cstdint>

class CheatManager {
public:
    CheatManager();

    void toggleDoubleDamage();
    void toggleInvincibility();
    void toggleDoubleFireRate();

    bool isDoubleDamageActive() const { return _doubleDamage; }
    bool isInvincibilityActive() const { return _invincibility; }
    bool isDoubleFireRateActive() const { return _doubleFireRate; }

    uint8_t getCheatFlags() const;

    void reset();

private:
    bool _doubleDamage;
    bool _invincibility;
    bool _doubleFireRate;
};
