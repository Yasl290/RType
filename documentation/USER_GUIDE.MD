# R-Type User Guide

## For Players

### Installation

#### Linux (Ubuntu/Debian)
```bash
# Install dependencies
sudo apt-get install build-essential cmake ninja-build \
    libx11-dev libxrandr-dev libxcursor-dev libxi-dev \
    libudev-dev libgl1-mesa-dev libopenal-dev \
    libvorbis-dev libflac-dev libfreetype6-dev

# Build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./build/game/r-type-client
./build/server/r-type_server  # In separate terminal
```

#### macOS
```bash
brew install cmake ninja
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/game/r-type-client
```

### Game Controls

| Action | Keys |
|--------|------|
| Move | Arrow Keys or ZQSD |
| Shoot (Normal) | Space (tap) |
| Shoot (Charged) | Space (hold 2s) |
| Pause | Escape |
| Toggle Music | M |
| Fullscreen | F11 |

### Gameplay Mechanics

#### Shooting System
- **Normal Shot**: 10 damage, fast rate of fire (0.25s cooldown)
- **Charged Shot**: 50 damage, pierces enemies, slower (1.5s cooldown)
- **Strategy**: Use normal shots for weak enemies, charged for groups

#### Enemy Types
- **Basic Enemy**: 50 HP, shoots projectiles every 2s
- **Contact Damage**: 20 HP lost on collision (destroys enemy)

#### Multiplayer
1. Start server: `./r-type_server` (default port 4242)
2. Connect clients: `./r-type-client` → Enter IP → Lobby
3. Press Enter/Space to ready up (all players must be ready)
4. Game starts automatically when all ready

### Troubleshooting

**Connection Failed**
```bash
# Check server is running
netstat -tuln | grep 4242

# Test with netcat
nc -u 127.0.0.1 4242
```

**Low FPS**
- Ensure VSync is enabled (capped at 60 FPS)
- Close background applications
- Check GPU drivers are updated

**Missing Textures**
```bash
# Verify assets directory
ls build/assets/sprites/

# If missing, rebuild with assets copy
cmake --build build --target r-type-client
```

## For Developers

### Quick Start: Using the Engine

#### 1. Create Your Game Class
```cpp
#include <engine/core/Registry.hpp>
#include <engine/graphics/Renderer.hpp>
#include <engine/systems/MovementSystem.hpp>

int main() {
    // Setup
    Renderer renderer(1280, 720, "My Game");
    Registry registry;
    MovementSystem movement;
    
    // Create entity
    EntityID player = registry.create();
    registry.add<Transform>(player, 100.f, 100.f);
    registry.add<Velocity>(player, 50.f, 0.f);
    
    // Game loop
    sf::Clock clock;
    while (renderer.isOpen()) {
        float dt = clock.restart().asSeconds();
        
        movement.update(registry, dt);
        
        renderer.clear(sf::Color::Black);
        // Render code...
        renderer.display();
    }
}
```

#### 2. Create Custom Components
```cpp
// MyComponent.hpp
#pragma once
#include <engine/core/Component.hpp>

class MyComponent : public Component {
public:
    MyComponent(float value) : data(value) {}
    ~MyComponent() override = default;
    
    float data;
};

// Usage
EntityID entity = registry.create();
registry.add<MyComponent>(entity, 42.0f);
```

#### 3. Create Custom Systems
```cpp
// MySystem.hpp
#pragma once
#include <engine/core/System.hpp>

class MySystem : public System {
public:
    void update(Registry& registry, float dt) override {
        registry.each<Transform, MyComponent>(
            [dt](EntityID id, Transform& t, MyComponent& c) {
                t.x += c.data * dt;
            }
        );
    }
};
```

### Engine API Reference

#### Registry
```cpp
// Entity lifecycle
EntityID create();                    // Create new entity
void destroy(EntityID entity);        // Immediate destruction
void markForDestruction(EntityID e);  // Deferred destruction
void cleanup();                       // Execute deferred operations

// Component operations
template<typename T, typename... Args>
T& add(EntityID entity, Args&&... args);  // Add component

template<typename T>
T& get(EntityID entity);                  // Get component reference

template<typename T>
bool has(EntityID entity);                // Check component exists

// Queries
template<typename... Comps, typename Func>
void each(Func func);                     // Iterate entities with components
```

**Example Query**:
```cpp
// Find all moving, visible entities
registry.each<Transform, Velocity, Sprite>(
    [](EntityID id, Transform& t, Velocity& v, Sprite& s) {
        // Process entity
    }
);
```

#### Built-in Components

**Transform** - Position and scale
```cpp
Transform(float x, float y);
float x, y, rotation, scaleX, scaleY;
```

**Velocity** - Movement vector
```cpp
Velocity(float vx, float vy);
float x, y;
```

**Sprite** - Visual representation
```cpp
Sprite(const std::string& texturePath);
bool loadTexture(const std::string& path);
void setScale(float scaleX, float scaleY);
sf::Sprite& getSprite();
```

**Controllable** - Player input
```cpp
Controllable(float speed);
float speed, shootCooldown, currentCooldown;
bool canShoot;
```

**Health** - Hit points
```cpp
Health(float maxHealth);
float current, max;
```

**Projectile** - Bullets
```cpp
Projectile(ProjectileType type, float damage, bool piercing);
ProjectileType type;  // Normal or Charged
float damage;
bool piercing;
bool isPlayerProjectile;
```

#### Built-in Systems

**MovementSystem** - Applies velocity to transform
```cpp
void update(Registry& registry, float dt) override;
// Updates Transform based on Velocity
```

**RenderSystem** - Draws sprites
```cpp
void render(Registry& registry, Renderer& renderer) override;
// Draws all entities with Transform + Sprite
```

**CollisionSystem** - Detects overlaps
```cpp
void update(Registry& registry, float dt) override;
// Checks bounding box intersections
```

**InputSystem** - Handles keyboard
```cpp
void update(Registry& registry, float dt) override;
// Sets Velocity on Controllable entities
```

### Advanced Usage

#### Custom Collision Detection
```cpp
class MyCollisionSystem : public System {
public:
    void update(Registry& registry, float dt) override {
        std::vector<std::pair<EntityID, EntityID>> collisions;
        
        // Broadphase
        registry.each<Transform, Collider>([&](EntityID e1, Transform& t1, Collider& c1) {
            registry.each<Transform, Collider>([&](EntityID e2, Transform& t2, Collider& c2) {
                if (e1 >= e2) return;  // Avoid duplicate checks
                
                if (overlaps(t1, c1, t2, c2)) {
                    collisions.push_back({e1, e2});
                }
            });
        });
        
        // Handle collisions
        for (auto [e1, e2] : collisions) {
            onCollision(registry, e1, e2);
        }
    }
    
private:
    bool overlaps(Transform& t1, Collider& c1, Transform& t2, Collider& c2) {
        // AABB intersection
        return t1.x < t2.x + c2.width && 
               t1.x + c1.width > t2.x &&
               t1.y < t2.y + c2.height && 
               t1.y + c1.height > t2.y;
    }
};
```

#### Entity Prefabs (Factory Pattern)
```cpp
EntityID createPlayer(Registry& registry, float x, float y) {
    EntityID player = registry.create();
    
    Sprite& sprite = registry.add<Sprite>(player);
    sprite.loadTexture("assets/sprites/player.png");
    sprite.getSprite().setTextureRect(sf::IntRect(0, 30, 316, 160));
    
    Transform& transform = registry.add<Transform>(player, x, y);
    transform.scaleX = 0.5f;
    transform.scaleY = 0.5f;
    
    registry.add<Velocity>(player, 0.f, 0.f);
    registry.add<Controllable>(player, 300.f);
    registry.add<Health>(player, 100.f);
    
    return player;
}

// Usage
EntityID player1 = createPlayer(registry, 100.f, 200.f);
EntityID player2 = createPlayer(registry, 100.f, 400.f);
```

#### System Manager Pattern
```cpp
class SystemManager {
public:
    void addSystem(std::unique_ptr<System> system) {
        _systems.push_back(std::move(system));
    }
    
    void update(Registry& registry, float dt) {
        for (auto& sys : _systems) {
            sys->update(registry, dt);
        }
    }
    
    void render(Registry& registry, Renderer& renderer) {
        for (auto& sys : _systems) {
            sys->render(registry, renderer);
        }
    }
    
private:
    std::vector<std::unique_ptr<System>> _systems;
};

// Usage
SystemManager manager;
manager.addSystem(std::make_unique<MovementSystem>());
manager.addSystem(std::make_unique<CollisionSystem>());
manager.addSystem(std::make_unique<RenderSystem>());

// Game loop
while (running) {
    float dt = getDeltaTime();
    manager.update(registry, dt);
    renderer.clear(sf::Color::Black);
    manager.render(registry, renderer);
    renderer.display();
}
```

### CMake Integration

Add engine to your project:
```cmake
# CMakeLists.txt
add_subdirectory(engine)

add_executable(my_game src/main.cpp)
target_link_libraries(my_game PRIVATE engine)
```

### Performance Tips

1. **Batch Operations**: Add multiple components to same entity consecutively
   ```cpp
   EntityID e = registry.create();
   registry.add<Transform>(e, 0, 0);    // Good: locality
   registry.add<Velocity>(e, 1, 1);
   registry.add<Sprite>(e, "tex.png");
   ```

2. **Avoid Mid-Iteration Destruction**:
   ```cpp
   // Use markForDestruction(), call cleanup() after loop
   ```

3. **Cache Systems**: Create systems once, reuse in game loop
   ```cpp
   // ✅ Good
   MovementSystem movement;
   while (running) {
       movement.update(registry, dt);
   }
   
   // ❌ Bad
   while (running) {
       MovementSystem().update(registry, dt);  // Heap allocation!
   }
   ```

4. **Minimize Component Count**: Only add necessary components
   - Sprites only for visible entities
   - Controllable only for player-controlled entities

### Example: Complete Mini-Game

```cpp
#include <engine/core/Registry.hpp>
#include <engine/graphics/Renderer.hpp>
#include <engine/systems/MovementSystem.hpp>
#include <engine/systems/RenderSystem.hpp>
#include <SFML/Window/Event.hpp>

int main() {
    Renderer renderer(800, 600, "Mini Game");
    Registry registry;
    
    MovementSystem movement;
    RenderSystem render;
    
    // Create player
    EntityID player = registry.create();
    registry.add<Transform>(player, 400.f, 300.f);
    registry.add<Velocity>(player, 0.f, 0.f);
    Sprite& pSprite = registry.add<Sprite>(player);
    pSprite.loadTexture("player.png");
    
    sf::Clock clock;
    while (renderer.isOpen()) {
        // Events
        sf::Event event;
        while (renderer.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                renderer.close();
        }
        
        // Input
        Velocity& vel = registry.get<Velocity>(player);
        vel.x = sf::Keyboard::isKeyPressed(sf::Keyboard::Right) ? 200.f : 0.f;
        vel.y = sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ? 200.f : 0.f;
        
        // Update
        float dt = clock.restart().asSeconds();
        movement.update(registry, dt);
        
        // Render
        renderer.clear(sf::Color::Black);
        render.render(registry, renderer);
        renderer.display();
    }
    
    return 0;
}
```

## Best Practices

1. **Separate Engine from Game Logic**: Keep engine/ generic, game-specific in game/
2. **Use Factory Functions**: Centralize entity creation for consistency
3. **Document System Dependencies**: Note which systems must run before others
4. **Test Component Interactions**: Unit test edge cases (empty pools, concurrent access)
5. **Profile Before Optimizing**: Use benchmarks to identify real bottlenecks

## Getting Help

- Check `docs/ECS_IMPLEMENTATION.md` for technical deep-dive
- See `tests/` directory for usage examples
- Review `game/src/main.cpp` for complete game loop