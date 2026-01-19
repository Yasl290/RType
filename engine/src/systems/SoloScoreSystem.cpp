#include <engine/systems/SoloScoreSystem.hpp>
#include <engine/gameplay/Enemy.hpp>
#include <engine/gameplay/Health.hpp>
#include <engine/gameplay/Score.hpp>
#include <engine/gameplay/Controllable.hpp>
#include <iostream>

SoloScoreSystem::SoloScoreSystem() = default;

void SoloScoreSystem::update(Registry& registry, float) {
    registry.each<Enemy, Health>([&](EntityID, Enemy& enemy, Health& health) {
        if (health.current <= 0.0f) {
            uint32_t points = 0;
            
            switch (enemy.type) {
                case EnemyType::Basic:
                    points = 100;
                    break;
                case EnemyType::Boss:
                    points = 1000;
                    break;
                default:
                    points = 100;
                    break;
            }
            
            registry.each<Controllable, Score>([points](EntityID, Controllable&, Score& score) {
                score.addPoints(points);
                score.incrementKills();
                std::cout << "[SoloScore] +" << points << " points! Total: " << score.getPoints() << std::endl;
            });
        }
    });
}