#pragma once

#include "engine/core/Component.hpp"
#include "engine/gameplay/UpgradeDefinitions.hpp"
#include <unordered_map>
#include <stdint.h>

struct PlayerStats : public Component {
    std::unordered_map<UpgradeType, int> upgradeLevels;
    int totalUpgradesApplied = 0;
    
    uint32_t currentScore = 0;
    uint32_t nextUpgradeThreshold = 500;
    uint32_t upgradeInterval = 500;
    
    float baseSpeed = 300.0f;
    float speedMultiplier = 1.0f;
    
    float fireRateMultiplier = 1.0f;
    float damageMultiplier = 1.0f;
    int multishotCount = 1;
    bool hasPiercingShots = false;
    
    float maxHealthBonus = 0.0f;
    float regenerationRate = 0.0f;
    float damageReduction = 0.0f;
    
    float scoreMultiplier = 1.0f;
    
    bool shouldShowUpgradeMenu() const {
        return currentScore >= nextUpgradeThreshold;
    }
    
    void applyUpgrade(UpgradeType type) {
        upgradeLevels[type]++;
        totalUpgradesApplied++;
        
        nextUpgradeThreshold += upgradeInterval;
        
        switch (type) {
            case UpgradeType::SPEED_BOOST:
                speedMultiplier += 0.15f;
                break;
            case UpgradeType::FIRE_RATE_UP:
                fireRateMultiplier += 0.20f;
                break;
            case UpgradeType::DAMAGE_UP:
                damageMultiplier += 0.25f;
                break;
            case UpgradeType::MULTISHOT:
                multishotCount++;
                break;
            case UpgradeType::PIERCING_SHOTS:
                hasPiercingShots = true;
                break;
            case UpgradeType::MAX_HP_UP:
                maxHealthBonus += 50.0f;
                break;
            case UpgradeType::REGENERATION:
                regenerationRate += 5.0f;
                break;
            case UpgradeType::DAMAGE_REDUCTION:
                damageReduction = std::min(0.75f, damageReduction + 0.10f);
                break;
            case UpgradeType::SCORE_MULTIPLIER:
                scoreMultiplier += 0.25f;
                break;
        }
    }
    
    int getUpgradeLevel(UpgradeType type) const {
        auto it = upgradeLevels.find(type);
        return (it != upgradeLevels.end()) ? it->second : 0;
    }
    
    bool canTakeUpgrade(UpgradeType type) const {
        const auto* def = UpgradeDatabase::getUpgrade(type);
        if (!def) return false;
        
        int currentLevel = getUpgradeLevel(type);
        return currentLevel < def->maxLevel;
    }
    
    float getFinalSpeed() const {
        return baseSpeed * speedMultiplier;
    }
    
    float getFinalFireRateDivisor() const {
        return 1.0f / fireRateMultiplier;
    }
    
    float getFinalScoreMultiplier() const {
        return scoreMultiplier;
    }
    
    float getHealthRegen() const {
        return regenerationRate;
    }
    
    float getFinalDamageMultiplier() const {
        return damageMultiplier;
    }
    
    float getMaxHealthBonus() const {
        return maxHealthBonus;
    }
};