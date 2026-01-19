#pragma once

#include "engine/gameplay/UpgradeDefinitions.hpp"
#include "engine/gameplay/PlayerStats.hpp"
#include <random>
#include <vector>

class UpgradeSelector {
public:
    UpgradeSelector();

    std::vector<UpgradeType> generateUpgradeChoices(const PlayerStats& stats, int numChoices = 3);
    
private:
    std::mt19937 _rng;
    std::vector<UpgradeType> getAvailableUpgrades(const PlayerStats& stats);
};