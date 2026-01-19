# ECS Implementation Guide

## Architecture Overview

```
┌─────────────────────────────────────────────────┐
│                   Registry                      │
│  - Manages entities (IDs)                       │
│  - Routes component operations to pools         │
│  - Provides query interface                     │
└──────────────┬──────────────────────────────────┘
               │
       ┌───────┴───────┐
       │               │
┌──────▼──────┐ ┌─────▼──────┐ ┌────────────┐
│ SparseSet<T>│ │SparseSet<V>│ │ SparseSet  │...
│  Transform  │ │  Velocity  │ │ Projectile │
└─────────────┘ └────────────┘ └────────────┘
```

## Core Components

### 1. Entity (Entity.hpp)

```cpp
using EntityID = uint32_t;
```

**Design Decision**: Simple uint32 ID instead of complex handles
- ✅ Trivially copyable
- ✅ Fits in CPU register
- ✅ Can index directly into sparse array
- ❌ No automatic invalidation (trade-off accepted)

### 2. SparseSet<T> (SparseSet.hpp)

The performance heart of the ECS.

```cpp
template<typename T>
class SparseSet : public IPool {
    std::vector<T> _dense;              // Packed components
    std::vector<EntityID> _denseToEntity; // Dense index -> Entity ID
    std::vector<size_t> _sparse;        // Entity ID -> Dense index
};
```

**Memory Layout**:
```
Entity IDs:  [5, 12, 3, 99]
Sparse:      [INVALID, INVALID, INVALID, 2, INVALID, 0, ..., INVALID, ..., 3]
                                         ↑           ↑                    ↑
Dense:       [Transform5, Transform12, Transform3, Transform99]
             └─────┬───────────┬───────────┬──────────┘
DenseToEntity:    [5,          12,         3,         99]
```

**Key Operations**:

```cpp
// O(1) Add - No reallocation of existing data
T& add(EntityID entity, Args&&... args) {
    if (entity >= _sparse.size())
        _sparse.resize(entity + 1, INVALID);
    
    size_t index = _dense.size();
    _sparse[entity] = index;
    _denseToEntity.push_back(entity);
    _dense.emplace_back(std::forward<Args>(args)...);
    return _dense.back();
}

// O(1) Remove - Swap-and-pop maintains density
void remove(EntityID entity) {
    size_t index = _sparse[entity];
    size_t last = _dense.size() - 1;
    
    // Swap with last element
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

// O(1) Get - Direct dense array access
T& get(EntityID entity) {
    return _dense[_sparse[entity]];
}

// O(1) Contains - Check sparse array
bool contains(EntityID entity) const {
    return entity < _sparse.size() && _sparse[entity] != INVALID;
}
```

**Why This Works**:
1. **Dense array iteration**: Systems iterate `_dense` directly (cache-friendly)
2. **O(1) random access**: `_sparse[entity]` gives dense index instantly
3. **Swap-and-pop removal**: Keeps dense array packed, no gaps
4. **Type erasure at pool level**: `IPool` interface for heterogeneous storage

### 3. Registry (Registry.hpp)

Central coordinator for all ECS operations.

```cpp
class Registry {
    std::vector<EntityID> _entities;
    std::vector<EntityID> _toDestroy;
    std::unordered_map<std::type_index, std::unique_ptr<IPool>> _pools;
    EntityID _nextID;
    
public:
    // Entity lifecycle
    EntityID create();
    void destroy(EntityID entity);
    void markForDestruction(EntityID entity); // Deferred deletion
    void cleanup();
    
    // Component operations (templated)
    template<typename T, typename... Args>
    T& add(EntityID entity, Args&&... args);
    
    template<typename T>
    T& get(EntityID entity);
    
    template<typename T>
    bool has(EntityID entity);
    
    // Query interface
    template<typename... Comps, typename Func>
    void each(Func func);
};
```

**Critical Implementation Details**:

#### Pool Management (Type Erasure)
```cpp
template<typename T>
SparseSet<T>& getPool() {
    std::type_index type = std::type_index(typeid(T));
    
    if (_pools.find(type) == _pools.end())
        _pools[type] = std::make_unique<SparseSet<T>>();
    
    return *static_cast<SparseSet<T>*>(_pools[type].get());
}
```

**Why this pattern**:
- Heterogeneous storage without vtable overhead for component access
- Only virtual calls during pool creation (rare)
- Component operations compile to direct function calls

#### Multi-Component Queries
```cpp
template<typename... Comps, typename Func>
void each(Func func) {
    for (EntityID entity : _entities) {
        if ((has<Comps>(entity) && ...))  // C++17 fold expression
            func(entity, get<Comps>(entity)...);
    }
}
```

**Optimization Note**: This iterates entities, not components. For sparse component sets, consider iterating the smallest pool:

```cpp
registry.each<Transform, RareComponent>([](EntityID id, auto& t, auto& r) {
});
```

### 4. Component (Component.hpp)

```cpp
class Component {
public:
    Component() = default;
    virtual ~Component() = default;
};
```

**Controversy**: Why virtual destructor if we avoid vtables?
- Components inherit from `Component` for type safety/documentation only
- Actual storage in `SparseSet<T>` uses concrete types
- Virtual destructor never called in hot path (pools destroyed at shutdown)

**Better Approach** (future improvement):
```cpp
template<typename T>
concept ComponentType = std::is_trivially_copyable_v<T>;
```

## System Architecture

### Base System (System.hpp)
```cpp
class System {
public:
    virtual ~System() = default;
    virtual void update(Registry& registry, float deltaTime) = 0;
    virtual void render(Registry& registry, Renderer& renderer);
};
```

### Example: MovementSystem
```cpp
class MovementSystem : public System {
public:
    void update(Registry& registry, float deltaTime) override {
        registry.each<Transform, Velocity>([deltaTime](
            EntityID, Transform& t, Velocity& v
        ) {
            t.x += v.x * deltaTime;
            t.y += v.y * deltaTime;
        });
    }
};
```

**Performance Profile**:
```
Assembly output (-O3):
  - Loop vectorized (SIMD instructions)
  - No virtual calls in hot loop
  - Cache prefetching triggered automatically
```

## Advanced Patterns

### Deferred Entity Destruction
```cpp
// ❌ BAD: Immediate destruction during iteration
registry.each<Health>([&](EntityID id, Health& h) {
    if (h.current <= 0)
        registry.destroy(id);
});

// ✅ GOOD: Mark for destruction, cleanup later
registry.each<Health>([&](EntityID id, Health& h) {
    if (h.current <= 0)
        registry.markForDestruction(id);
});
registry.cleanup();
```

### Component Dependencies
```cpp
EntityID entity = registry.create();
registry.add<Transform>(entity, 0.f, 0.f);
Sprite& sprite = registry.add<Sprite>(entity);
sprite.loadTexture("player.png");
```

**Future Enhancement**: Automatic dependency injection
```cpp
template<typename T>
struct ComponentTraits;

template<>
struct ComponentTraits<Sprite> {
    using Dependencies = std::tuple<Transform>;
};
```

## Memory Management

### Texture Caching (Sprite.cpp)
```cpp
static std::unordered_map<std::string, std::shared_ptr<sf::Texture>> g_textureCache;

bool Sprite::loadTexture(const std::string& path) {
    auto it = g_textureCache.find(path);
    if (it != g_textureCache.end()) {
        texture = it->second;  // Shared ownership
        sprite.setTexture(*texture, true);
        return true;
    }
    // Load and cache...
}
```

**Impact**: 100 enemies with same texture = 1 texture in memory

## Testing Strategy

### Unit Tests (59 tests)
```cpp
TEST(RegistryTest, CreateEntity) {
    Registry registry;
    EntityID e1 = registry.create();
    EntityID e2 = registry.create();
    EXPECT_NE(e1, e2);
}

TEST(SparseSetTest, SwapAndPopMaintainsDensity) {
    SparseSet<Transform> pool;
    EntityID e1 = 5, e2 = 12, e3 = 99;
    
    pool.add(e1, 1.f, 1.f);
    pool.add(e2, 2.f, 2.f);
    pool.add(e3, 3.f, 3.f);
    
    pool.remove(e2);  // Should swap e3 into e2's slot
    
    EXPECT_TRUE(pool.contains(e1));
    EXPECT_FALSE(pool.contains(e2));
    EXPECT_TRUE(pool.contains(e3));
    EXPECT_EQ(pool.get(e1).x, 1.f);
    EXPECT_EQ(pool.get(e3).x, 3.f);
}
```

## Common Pitfalls

### 1. Entity Reference Invalidation
```cpp
// ❌ DANGER: `entity` becomes invalid if vector reallocates
EntityID entity = registry.create();
Transform& t = registry.add<Transform>(entity, 0.f, 0.f);
for (int i = 0; i < 1000; ++i) {
    registry.create();  // Vector reallocation may occur!
}
t.x = 100;  // MAY CRASH if entity moved in memory
```

**Solution**: Always use EntityID, never store references across operations that might reallocate.

### 2. System Update Order Matters
```cpp
// ❌ BAD ORDER
systems.push_back(std::make_unique<RenderSystem>());
systems.push_back(std::make_unique<MovementSystem>());
// Renders old positions!

// ✅ CORRECT ORDER
systems.push_back(std::make_unique<MovementSystem>());
systems.push_back(std::make_unique<RenderSystem>());
```

### 3. Forgetting Cleanup
```cpp
// ❌ Memory leak
void update(Registry& reg, float dt) {
    for (auto& sys : systems)
        sys->update(reg, dt);
    // No cleanup call!
}

// ✅ Proper lifecycle
void update(Registry& reg, float dt) {
    for (auto& sys : systems)
        sys->update(reg, dt);
    reg.cleanup();  // Destroy marked entities
}
```

## Future Optimizations

1. **Archetype-based storage**: Group entities by component signature
2. **Parallel system execution**: Systems with no conflicts can run concurrently
3. **SIMD-friendly memory layout**: Struct-of-Arrays instead of Array-of-Structs
4. **Memory pooling**: Pre-allocate entity blocks to avoid fragmentation

## Further Reading

- [Data-Oriented Design](http://www.dataorienteddesign.com/dodmain/)
- [EnTT Documentation](https://github.com/skypjack/entt) (inspiration for this implementation)
- [CppCon: ECS Back and Forth](https://www.youtube.com/watch?v=W3aieHjyNvw)