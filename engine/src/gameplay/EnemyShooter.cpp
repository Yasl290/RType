#include "engine/gameplay/EnemyShooter.hpp"

EnemyShooter::EnemyShooter()
    : shootInterval(2.0f), currentCooldown(0.f), bulletSpeed(250.f)
{
}

EnemyShooter::EnemyShooter(float shootInterval, float bulletSpeed)
    : shootInterval(shootInterval), currentCooldown(0.f), bulletSpeed(bulletSpeed)
{
}

EnemyShooter EnemyShooter::createForType(EnemyType type)
{
    switch (type) {
        case EnemyType::FastShooter:
            return EnemyShooter(1.0f, 300.f);
        case EnemyType::Bomber:
            return EnemyShooter(3.0f, 200.f);
        case EnemyType::Basic:
        default:
            return EnemyShooter(2.0f, 250.f);
    }
}