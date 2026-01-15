#pragma once

template<typename T>
SparseSet<T>& Registry::getPool()
{
    std::type_index type = std::type_index(typeid(T));

    if (_pools.find(type) == _pools.end())
        _pools[type] = std::make_unique<SparseSet<T>>();

    return *static_cast<SparseSet<T>*>(_pools[type].get());
}

template<typename T, typename... Args>
T& Registry::add(EntityID entity, Args&&... args)
{
    return getPool<T>().add(entity, std::forward<Args>(args)...);
}

template<typename T>
T& Registry::get(EntityID entity)
{
    return getPool<T>().get(entity);
}

template<typename T>
bool Registry::has(EntityID entity)
{
    std::type_index type = std::type_index(typeid(T));
    auto it = _pools.find(type);
    if (it == _pools.end())
        return false;
    return static_cast<SparseSet<T>*>(it->second.get())->contains(entity);
}

template<typename... Comps, typename Func>
void Registry::each(Func func)
{
    for (EntityID entity : _entities) {
        if ((has<Comps>(entity) && ...))
            func(entity, get<Comps>(entity)...);
    }
}