#include "engine/gameplay/UpgradeSelector.hpp"
#include <algorithm>
#include <iostream>

UpgradeSelector::UpgradeSelector()
    : _rng(std::random_device{}())
{
}

std::vector<UpgradeType> UpgradeSelector::generateUpgradeChoices(const PlayerStats& stats, int numChoices) {
    std::vector<UpgradeType> available = getAvailableUpgrades(stats);
    
    if (available.empty()) {
        std::cout << "[UpgradeSelector] No upgrades available!" << std::endl;
        return {};
    }
    std::shuffle(available.begin(), available.end(), _rng);
    int actualChoices = std::min(numChoices, static_cast<int>(available.size()));
    std::vector<UpgradeType> choices(available.begin(), available.begin() + actualChoices);
    
    std::cout << "[UpgradeSelector] Generated " << choices.size() << " upgrade choices" << std::endl;
    
    return choices;
}

std::vector<UpgradeType> UpgradeSelector::getAvailableUpgrades(const PlayerStats& stats) {
    std::vector<UpgradeType> available;
    
    for (const auto& upgrade : UpgradeDatabase::getAllUpgrades()) {
        if (stats.canTakeUpgrade(upgrade.type)) {
            available.push_back(upgrade.type);
        }
    }
    
    return available;
}