#include "engine/gameplay/EnemyShooter.hpp"

EnemyShooter::EnemyShooter()
    : shootInterval(2.0f), currentCooldown(0.f), bulletSpeed(250.f)
{
}

EnemyShooter::EnemyShooter(float shootInterval, float bulletSpeed)
    : shootInterval(shootInterval), currentCooldown(0.f), bulletSpeed(bulletSpeed)
{
}