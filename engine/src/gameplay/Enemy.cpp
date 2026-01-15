#include "engine/gameplay/Enemy.hpp"

Enemy::Enemy()
    : type(EnemyType::Basic)
{
}

Enemy::Enemy(EnemyType type)
    : type(type)
{
}