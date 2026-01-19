#include "engine/gameplay/Score.hpp"

Score::Score(uint32_t initialPoints, uint32_t initialKills)
    : _points(initialPoints)
    , _enemiesKilled(initialKills)
{
}

void Score::addPoints(uint32_t amount)
{
    _points += amount;
}

void Score::incrementKills()
{
    _enemiesKilled++;
}

void Score::reset()
{
    _points = 0;
    _enemiesKilled = 0;
}

uint32_t Score::getPoints() const
{
    return _points;
}

uint32_t Score::getEnemiesKilled() const
{
    return _enemiesKilled;
}