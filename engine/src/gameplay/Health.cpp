#include "engine/gameplay/Health.hpp"

Health::Health()
    : current(100.f), max(100.f)
{
}

Health::Health(float maxHealth)
    : current(maxHealth), max(maxHealth)
{
}