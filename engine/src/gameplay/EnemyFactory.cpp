#include "engine/gameplay/EnemyFactory.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Enemy.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/EnemyShooter.hpp"

EntityID createBasicEnemy(Registry& registry, float x, float y)
{
    EntityID enemy = registry.create();

    Sprite& sprite = registry.add<Sprite>(enemy);
    sprite.loadTexture("assets/sprites/enemy_basic.png");

    Transform& transform = registry.add<Transform>(enemy, x, y);
    transform.scaleX = 0.5f;
    transform.scaleY = 0.5f;

    registry.add<Velocity>(enemy, -150.f, 0.f);
    registry.add<Enemy>(enemy, EnemyType::Basic);
    registry.add<Health>(enemy, 50.f);
    registry.add<EnemyShooter>(enemy, 2.0f, 250.f);

    return enemy;
}