#pragma once

#include "Entity.hpp"
#include "SparseSet.hpp"
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>

class Registry {
public:
    Registry();
    ~Registry();

    EntityID create();
    void destroy(EntityID entity);
    void markForDestruction(EntityID entity);
    void cleanup();

    template<typename T, typename... Args>
    T& add(EntityID entity, Args&&... args);

    template<typename T>
    T& get(EntityID entity);

    template<typename T>
    bool has(EntityID entity);

    template<typename... Comps, typename Func>
    void each(Func func);

private:
    std::vector<EntityID> _entities;
    std::vector<EntityID> _toDestroy;
    std::unordered_map<std::type_index, std::unique_ptr<IPool>> _pools;
    EntityID _nextID;

    template<typename T>
    SparseSet<T>& getPool();
};

#include "Registry.inl"