#pragma once

#include <string>
#include <vector>

enum class UpgradeType {
    SPEED_BOOST,
    FIRE_RATE_UP,
    DAMAGE_UP,
    MULTISHOT,
    PIERCING_SHOTS,
    MAX_HP_UP,
    REGENERATION,
    DAMAGE_REDUCTION,
    SCORE_MULTIPLIER
};

struct UpgradeDefinition {
    UpgradeType type;
    std::string name;
    std::string description;
    int maxLevel;
    std::string iconPath;
};

class UpgradeDatabase {
public:
    static const std::vector<UpgradeDefinition>& getAllUpgrades() {
        static const std::vector<UpgradeDefinition> upgrades = {
            {UpgradeType::SPEED_BOOST, "Faster Thrusters", "+15% Movement Speed", 5, ""},
            {UpgradeType::FIRE_RATE_UP, "Rapid Fire", "+20% Fire Rate", 5, ""},
            {UpgradeType::DAMAGE_UP, "Plasma Upgrade", "+25% Damage", 5, ""},
            {UpgradeType::MULTISHOT, "Dual Cannons", "+1 Projectile", 3, ""},
            {UpgradeType::PIERCING_SHOTS, "Armor Piercing", "Bullets pierce enemies", 1, ""},
            {UpgradeType::MAX_HP_UP, "Hull Reinforcement", "+50 Max HP", 5, ""},
            {UpgradeType::REGENERATION, "Auto Repair", "Regenerate 5 HP/sec", 3, ""},
            {UpgradeType::DAMAGE_REDUCTION, "Armor Plating", "-10% Damage Taken", 5, ""},
            {UpgradeType::SCORE_MULTIPLIER, "Score Boost", "+25% Score", 5, ""}
        };
        return upgrades;
    }
    
    static const UpgradeDefinition* getUpgrade(UpgradeType type) {
        for (const auto& upgrade : getAllUpgrades()) {
            if (upgrade.type == type) {
                return &upgrade;
            }
        }
        return nullptr;
    }
};