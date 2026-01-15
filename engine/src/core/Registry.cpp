#include "engine/core/Registry.hpp"
#include <algorithm>

Registry::Registry() : _nextID(0)
{
}

Registry::~Registry() = default;

EntityID Registry::create()
{
    EntityID entity = _nextID++;
    _entities.push_back(entity);
    return entity;
}

void Registry::destroy(EntityID entity)
{
    for (auto& pair : _pools)
        pair.second->remove(entity);

    auto it = std::find(_entities.begin(), _entities.end(), entity);
    if (it != _entities.end())
        _entities.erase(it);
}

void Registry::markForDestruction(EntityID entity)
{
    _toDestroy.push_back(entity);
}

void Registry::cleanup()
{
    for (EntityID entity : _toDestroy)
        destroy(entity);
    _toDestroy.clear();
}