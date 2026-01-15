#pragma once

#include "Entity.hpp"
#include <vector>
#include <limits>

class IPool {
public:
    virtual ~IPool() = default;
    virtual void remove(EntityID entity) = 0;
};

template<typename T>
class SparseSet : public IPool {
public:
    template<typename... Args>
    T& add(EntityID entity, Args&&... args);

    void remove(EntityID entity) override;
    T& get(EntityID entity);
    bool contains(EntityID entity) const;

private:
    std::vector<T> _dense;
    std::vector<EntityID> _denseToEntity;
    std::vector<std::size_t> _sparse;

    static constexpr std::size_t INVALID = std::numeric_limits<std::size_t>::max();
};

#include "SparseSet.inl"