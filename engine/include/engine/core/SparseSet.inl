#pragma once

#include <utility>

template<typename T>
template<typename... Args>
T& SparseSet<T>::add(EntityID entity, Args&&... args)
{
    if (entity >= _sparse.size())
        _sparse.resize(entity + 1, INVALID);

    std::size_t index = _dense.size();
    _sparse[entity] = index;
    _denseToEntity.push_back(entity);
    _dense.emplace_back(std::forward<Args>(args)...);
    return _dense.back();
}

template<typename T>
void SparseSet<T>::remove(EntityID entity)
{
    if (!contains(entity))
        return;

    std::size_t index = _sparse[entity];
    std::size_t last = _dense.size() - 1;

    if (index != last) {
        EntityID lastEntity = _denseToEntity[last];
        _dense[index] = std::move(_dense[last]);
        _denseToEntity[index] = lastEntity;
        _sparse[lastEntity] = index;
    }
    _dense.pop_back();
    _denseToEntity.pop_back();
    _sparse[entity] = INVALID;
}

template<typename T>
T& SparseSet<T>::get(EntityID entity)
{
    return _dense[_sparse[entity]];
}

template<typename T>
bool SparseSet<T>::contains(EntityID entity) const
{
    return entity < _sparse.size() && _sparse[entity] != INVALID;
}