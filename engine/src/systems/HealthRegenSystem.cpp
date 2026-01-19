#include <engine/systems/HealthRegenSystem.hpp>
#include <engine/gameplay/Controllable.hpp>
#include <engine/gameplay/Health.hpp>
#include <engine/gameplay/PlayerStats.hpp>
#include <algorithm>

HealthRegenSystem::HealthRegenSystem() 
    : _regenTimer(0.0f)
{
}

void HealthRegenSystem::update(Registry& registry, float dt) {
    _regenTimer += dt;
    
    if (_regenTimer >= 1.0f) {
        _regenTimer = 0.0f;
        
        registry.each<Controllable, Health, PlayerStats>([](EntityID, Controllable&, Health& health, PlayerStats& stats) {
            float maxHPBonus = stats.getMaxHealthBonus();
            if (maxHPBonus > 0.0f) {
                health.max = 100.0f + maxHPBonus;
            }
            
            float regenAmount = stats.getHealthRegen();
            if (regenAmount > 0.0f && health.current < health.max) {
                health.current = std::min(health.max, health.current + regenAmount);
            }
        });
    }
}