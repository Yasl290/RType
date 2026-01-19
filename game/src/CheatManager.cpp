#include "CheatManager.hpp"
#include <iostream>

CheatManager::CheatManager()
    : _doubleDamage(false)
    , _invincibility(false)
    , _doubleFireRate(false)
{
}

void CheatManager::toggleDoubleDamage()
{
    _doubleDamage = !_doubleDamage;
    std::cout << "[CheatManager] Double Damage: "
              << (_doubleDamage ? "ON" : "OFF") << std::endl;
}

void CheatManager::toggleInvincibility()
{
    _invincibility = !_invincibility;
    std::cout << "[CheatManager] Invincibility: "
              << (_invincibility ? "ON" : "OFF") << std::endl;
}

void CheatManager::toggleDoubleFireRate()
{
    _doubleFireRate = !_doubleFireRate;
    std::cout << "[CheatManager] Double Fire Rate: "
              << (_doubleFireRate ? "ON" : "OFF") << std::endl;
}

uint8_t CheatManager::getCheatFlags() const
{
    uint8_t flags = 0;
    if (_doubleDamage) flags |= (1 << 5);
    if (_invincibility) flags |= (1 << 6);
    if (_doubleFireRate) flags |= (1 << 7);
    return flags;
}

void CheatManager::reset()
{
    _doubleDamage = false;
    _invincibility = false;
    _doubleFireRate = false;
    std::cout << "[CheatManager] All cheats reset" << std::endl;
}
