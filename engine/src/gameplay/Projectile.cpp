#include "engine/gameplay/Projectile.hpp"

Projectile::Projectile()
    : type(ProjectileType::Normal), damage(10.f), piercing(false), isPlayerProjectile(true),  ownerId(0)
{
}

Projectile::Projectile(ProjectileType type, float damage, bool piercing, uint32_t ownerId)
    : type(type), damage(damage), piercing(piercing), isPlayerProjectile(true), ownerId(ownerId)
{
}