# Developer Documentation

## Project Architecture

```
r-type/
├── engine/              # Reusable game engine
│   ├── include/engine/
│   │   ├── core/       # ECS (Registry, Entity, SparseSet)
│   │   ├── systems/    # Built-in systems
│   │   ├── graphics/   # Rendering (Sprite, Animation)
│   │   ├── physics/    # Transform, Velocity
│   │   ├── gameplay/   # Game components
│   │   └── audio/      # AudioManager
│   └── src/            # Implementation files
│
├── game/               # R-Type client
│   ├── src/
│   │   ├── main.cpp           # Client entry point
│   │   ├── Menu.cpp           # Main menu
│   │   ├── NetworkClient.cpp # UDP client
│   │   └── ui/                # UI screens
│   └── assets/         # Game resources
│
├── server/             # R-Type server
│   ├── src/
│   │   ├── main.cpp        # Server entry point
│   │   ├── Server.cpp      # Game server logic
│   │   ├── NetworkModule.cpp  # UDP networking
│   │   └── GameModule.cpp     # Server-side game state
│   └── include/
│       └── protocol/   # Network protocol definitions
│
└── tests/              # Unit tests (GTest)
```

## Separation of Concerns

| Layer | Responsibility | Examples |
|-------|---------------|----------|
| **Engine** | Generic game functionality | ECS, rendering, input, audio |
| **Game** | R-Type specific logic | Menus, networking, UI |
| **Server** | Authoritative game state | Physics, collision, spawning |

**Rule**: Engine must remain game-agnostic. No R-Type specific code in `engine/`.

## Code Style

### Naming Conventions

```cpp
// Classes: PascalCase
class MovementSystem;
class Transform;

// Member variables: _camelCase with underscore
class Player {
    float _health;
    std::string _name;
};

// Functions/methods: camelCase
void updatePosition(float dt);
bool hasComponent(EntityID id);

// Constants: UPPER_CASE
constexpr float GRAVITY = 9.81f;
static constexpr size_t MAX_ENTITIES = 10000;

// Files: Match class name
// Transform.hpp, Transform.cpp
// MovementSystem.hpp, MovementSystem.cpp
```

### Header/Implementation Split

**Always** separate declarations from definitions:

```cpp
// Transform.hpp
#pragma once
#include <engine/core/Component.hpp>

class Transform : public Component {
public:
    Transform();
    Transform(float x, float y);
    ~Transform() override = default;

    float x, y, rotation, scaleX, scaleY;
};

// Transform.cpp
#include "engine/physics/Transform.hpp"

Transform::Transform()
    : x(0.f), y(0.f), rotation(0.f), scaleX(1.f), scaleY(1.f)
{}

Transform::Transform(float x, float y)
    : x(x), y(y), rotation(0.f), scaleX(1.f), scaleY(1.f)
{}
```

**Exception**: Template code must stay in headers (or `.inl` files).

### Include Guards

Use `#pragma once` (modern, simpler):
```cpp
#pragma once
// Header content
```

### Includes Order

```cpp
// 1. Corresponding header (for .cpp files)
#include "Transform.hpp"

// 2. Engine headers
#include <engine/core/Registry.hpp>
#include <engine/systems/System.hpp>

// 3. Third-party libraries
#include <SFML/Graphics.hpp>

// 4. Standard library
#include <vector>
#include <memory>
#include <iostream>
```

## Git Workflow

### Branch Strategy

```
main
  ├── develop
  │   ├── feat/RTYPE-123-player-movement
  │   ├── feat/RTYPE-124-enemy-ai
  │   └── fix/RTYPE-125-collision-bug
```

- **main**: Production-ready releases
- **develop**: Integration branch
- **feat/RTYPE-X-description**: New features
- **fix/RTYPE-X-description**: Bug fixes

### Commit Messages

Format: `<type>(scope): <description>`

```bash
feat(engine): add SparseSet component storage
fix(server): resolve entity cleanup segfault
docs(readme): update build instructions
test(ecs): add Registry multi-component query tests
refactor(movement): extract velocity calculation
```

**Types**: `feat`, `fix`, `docs`, `test`, `refactor`, `style`, `perf`

### Pull Request Template

```markdown
## Description
Brief summary of changes.

## Related Issue
Closes #123

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
- [ ] Unit tests added/updated
- [ ] Manual testing performed
- [ ] CI/CD passes

## Checklist
- [ ] Code follows style guidelines
- [ ] Self-reviewed code
- [ ] Commented complex sections
- [ ] Documentation updated
```

## Testing Strategy

### Unit Tests (GTest)

```cpp
// tests/test_registry.cpp
#include <gtest/gtest.h>
#include <engine/core/Registry.hpp>
#include <engine/physics/Transform.hpp>

TEST(RegistryTest, CreateEntity) {
    Registry registry;
    EntityID e1 = registry.create();
    EntityID e2 = registry.create();
    
    EXPECT_NE(e1, e2);
    EXPECT_GE(e1, 0);
    EXPECT_GE(e2, 0);
}

TEST(RegistryTest, AddComponent) {
    Registry registry;
    EntityID entity = registry.create();
    
    Transform& t = registry.add<Transform>(entity, 10.f, 20.f);
    
    EXPECT_EQ(t.x, 10.f);
    EXPECT_EQ(t.y, 20.f);
    EXPECT_TRUE(registry.has<Transform>(entity));
}

TEST(RegistryTest, GetComponentThrowsIfMissing) {
    Registry registry;
    EntityID entity = registry.create();
    
    EXPECT_THROW(registry.get<Transform>(entity), std::exception);
}
```

### Test Coverage Goals

- **Core ECS**: 95%+ (critical path)
- **Systems**: 80%+
- **Game logic**: 60%+
- **UI**: 40%+ (manual testing primary)

Run tests:
```bash
cd build
./tests/rtype_tests
```

### Integration Testing

Test system interactions:
```cpp
TEST(IntegrationTest, MovementSystemUpdatesTransform) {
    Registry registry;
    MovementSystem movement;
    
    EntityID entity = registry.create();
    registry.add<Transform>(entity, 0.f, 0.f);
    registry.add<Velocity>(entity, 10.f, 5.f);
    
    movement.update(registry, 0.1f);  // dt = 0.1s
    
    Transform& t = registry.get<Transform>(entity);
    EXPECT_FLOAT_EQ(t.x, 1.0f);  // 10 * 0.1
    EXPECT_FLOAT_EQ(t.y, 0.5f);  // 5 * 0.1
}
```

## CI/CD Pipeline

### GitHub Actions (.github/workflows/ci.yml)

```yaml
jobs:
  build-and-test-linux:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4
        
      - name: Install Dependencies
        run: sudo apt-get install [deps...]
        
      - name: Build
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build
          
      - name: Run Tests
        run: ./build/tests/rtype_tests
        
      - name: Upload Artifacts
        uses: actions/upload-artifact@v4
```

**Triggers**: Push to `main`/`develop`, pull requests

**Checks**:
1. ✅ Compiles on Linux/macOS
2. ✅ All tests pass
3. ✅ Clang-tidy (code quality)
4. ✅ Code coverage >70%

## Network Protocol

### Packet Structure

```cpp
struct PacketHeader {
    uint32_t magic;              // 0x52545950 ("RTYP")
    PacketType type;             // Enum packet type
    uint16_t payload_size;       // Bytes after header
    uint32_t sequence_number;    // For ordering/ack
} __attribute__((packed));
```

### Adding New Packets

1. **Define in Protocol.hpp**:
```cpp
enum class PacketType : uint8_t {
    // ... existing
    NEW_PACKET = 0xNN,
};

struct NewPacket {
    PacketHeader header;
    uint32_t field1;
    float field2;
    
    NewPacket() : field1(0), field2(0.f) {
        header.type = PacketType::NEW_PACKET;
        header.payload_size = sizeof(NewPacket) - sizeof(PacketHeader);
    }
} __attribute__((packed));
```

2. **Handle in Server.cpp**:
```cpp
void Server::handleMessage(const ReceivedMessage& msg) {
    switch (msg.packet_type) {
        case PacketType::NEW_PACKET:
            handleNewPacket(msg);
            break;
    }
}

void Server::handleNewPacket(const ReceivedMessage& msg) {
    NewPacket packet;
    std::memcpy(&packet, msg.payload.data(), sizeof(NewPacket));
    
    // Process packet
}
```

3. **Handle in NetworkClient.cpp**:
```cpp
void NetworkClient::handlePacket(const uint8_t* data, size_t size) {
    switch (header.type) {
        case PacketType::NEW_PACKET:
            // Handle on client side
            break;
    }
}
```

## Performance Profiling

### Using perf (Linux)
```bash
# Build with debug symbols
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build

# Profile
perf record -g ./build/game/r-type-client
perf report

# Flamegraph
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

### Valgrind (Memory Leaks)
```bash
valgrind --leak-check=full --show-leak-kinds=all \
    ./build/tests/rtype_tests
```

### Expected Hotspots
1. `Registry::each<>()` - Component iteration
2. `CollisionSystem::update()` - N² collision checks
3. `RenderSystem::render()` - GPU draw calls

## Common Issues & Solutions

### Issue: Segfault in Registry::get<>()
**Cause**: Entity doesn't have component
**Solution**: Always check `has<>()` first or use try-catch

```cpp
// Option 1: Check first
if (registry.has<Transform>(entity)) {
    Transform& t = registry.get<Transform>(entity);
}

// Option 2: Try-catch
try {
    Transform& t = registry.get<Transform>(entity);
} catch (const std::exception& e) {
    // Handle missing component
}
```

### Issue: Iterator Invalidation
**Cause**: Modifying container during iteration
**Solution**: Use `markForDestruction()`, cleanup after

```cpp
// ❌ BAD
registry.each<Health>([&](EntityID id, Health& h) {
    if (h.current <= 0)
        registry.destroy(id);  // CRASH!
});

// ✅ GOOD
registry.each<Health>([&](EntityID id, Health& h) {
    if (h.current <= 0)
        registry.markForDestruction(id);
});
registry.cleanup();
```

### Issue: Network Desync
**Cause**: Client predicts state differently than server
**Solution**: Server is authoritative, client interpolates

```cpp
// Client: Apply server position smoothly
void applyServerUpdate(Transform& clientTransform, float serverX, float serverY) {
    // Interpolate instead of snap
    clientTransform.x += (serverX - clientTransform.x) * 0.3f;
    clientTransform.y += (serverY - clientTransform.y) * 0.3f;
}
```

## Code Review Checklist

Before submitting PR:

**Functionality**
- [ ] Feature works as intended
- [ ] No regressions in existing features
- [ ] Edge cases handled

**Code Quality**
- [ ] Follows naming conventions
- [ ] No magic numbers (use constants)
- [ ] Complex logic commented
- [ ] No code duplication

**Performance**
- [ ] No unnecessary allocations in hot paths
- [ ] Efficient data structures chosen
- [ ] No N² algorithms unless necessary

**Testing**
- [ ] Unit tests added for new code
- [ ] All tests pass locally
- [ ] Manual testing performed

**Documentation**
- [ ] Public APIs documented
- [ ] README updated if needed
- [ ] CHANGELOG entry added

## Debugging Tips

### GDB Workflow
```bash
# Build with debug symbols
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run with GDB
gdb ./build/game/r-type-client
(gdb) break main
(gdb) run
(gdb) next
(gdb) print variable
(gdb) backtrace
```

### Useful GDB Commands
```
info locals          # Show local variables
info args            # Show function arguments
frame <n>            # Switch stack frame
watch variable       # Break when variable changes
condition 1 x > 100  # Conditional breakpoint
```

### Logging Best Practices
```cpp
// Use consistent prefixes
std::cout << "[MovementSystem] Entity " << id << " moved to (" 
          << t.x << ", " << t.y << ")" << std::endl;

// Log levels (manual implementation)
#define LOG_DEBUG(msg) if (DEBUG_MODE) std::cout << "[DEBUG] " << msg << std::endl
#define LOG_INFO(msg)  std::cout << "[INFO] " << msg << std::endl
#define LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl

// Usage
LOG_INFO("Server started on port " << port);
LOG_ERROR("Failed to load texture: " << path);
```

## Build System

### CMake Targets

```bash
cmake --build build --target engine          # Build engine only
cmake --build build --target r-type-client   # Build client
cmake --build build --target r-type_server   # Build server
cmake --build build --target rtype_tests     # Build tests
```

### Adding New Source Files

**Engine**:
```cmake
# engine/CMakeLists.txt
add_library(engine STATIC
    src/core/Registry.cpp
    src/systems/NewSystem.cpp  # Add here
)
```

**Game**:
```cmake
# game/CMakeLists.txt
add_executable(r-type-client
    src/main.cpp
    src/NewFeature.cpp  # Add here
)
```

## Release Process

1. **Version Bump**: Update `CMakeLists.txt`
   ```cmake
   project(RType VERSION 1.1.0 LANGUAGES CXX)
   ```

2. **Changelog**: Update `CHANGELOG.md`
   ```markdown
   ## [1.1.0] - 2025-01-15
   ### Added
   - New enemy type: FastEnemy
   ### Fixed
   - Collision detection false positives
   ```

3. **Tag Release**:
   ```bash
   git tag -a v1.1.0 -m "Release 1.1.0"
   git push origin v1.1.0
   ```

4. **GitHub Actions**: Automatically builds and uploads artifacts

## Resources

- **ECS Pattern**: [Data-Oriented Design Book](http://www.dataorienteddesign.com/dodmain/)
- **SFML Docs**: https://www.sfml-dev.org/documentation/
- **CMake Guide**: https://cmake.org/cmake/help/latest/guide/tutorial/
- **GTest Primer**: https://google.github.io/googletest/primer.html
- **Networking**: [Gaffer on Games](https://gafferongames.com/)