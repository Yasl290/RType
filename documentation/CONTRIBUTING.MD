# Contributing to R-Type

Thank you for your interest in contributing! This document provides guidelines for contributing to the R-Type project.

## Quick Start

1. **Fork the repository** on GitHub
2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/r-type.git
   cd r-type
   ```
3. **Create a branch**:
   ```bash
   git checkout -b feat/RTYPE-XXX-your-feature
   ```
4. **Make changes, test, commit**
5. **Push and create Pull Request**

## What Can You Contribute?

### 🐛 Bug Fixes
- Crash reports with reproducible steps
- Memory leaks or performance issues
- Incorrect behavior

### ✨ Features
- New enemy types
- Additional power-ups
- UI improvements
- Network protocol enhancements

### 📚 Documentation
- Tutorial improvements
- API documentation
- Code examples
- Translation

### 🧪 Tests
- Increase test coverage
- Edge case scenarios
- Performance benchmarks

## Before You Start

### Check Existing Issues
Search [GitHub Issues](https://github.com/YOUR_ORG/r-type/issues) to avoid duplicates.

### Discuss Major Changes
For significant features, open an issue first to discuss:
- Is this feature aligned with project goals?
- What's the proposed implementation?
- Are there alternatives?

## Development Setup

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake ninja-build \
    libx11-dev libxrandr-dev libxi-dev libudev-dev \
    libgl1-mesa-dev libopenal-dev libfreetype6-dev

# macOS
brew install cmake ninja
```

### Build
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/tests/rtype_tests  # Run tests
```

### IDE Setup

**VSCode** (`.vscode/settings.json`):
```json
{
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
  "files.associations": {
    "*.hpp": "cpp",
    "*.inl": "cpp"
  }
}
```

**CLion**: Open project, CLion auto-detects CMake

## Code Standards

### Style Guide

**Formatting**: Follow existing code style
- Indentation: 4 spaces (no tabs)
- Braces: K&R style
- Line length: 100 characters max

**Naming**:
```cpp
class MyClass;                   // PascalCase
void myFunction();               // camelCase
float _memberVariable;           // _camelCase (underscore prefix)
const float MY_CONSTANT = 3.14f; // UPPER_CASE
```

**Comments**:
```cpp
// Single-line comment for brief explanations

/**
 * Multi-line comment for:
 * - Public API documentation
 * - Complex algorithms
 * - Non-obvious design decisions
 */
```

### Code Quality

**Avoid**:
- Magic numbers: `if (health < 50)` → `if (health < LOW_HEALTH_THRESHOLD)`
- Deep nesting: Refactor to early returns
- Long functions: Keep under 50 lines
- Global state: Prefer dependency injection

**Prefer**:
- RAII for resource management
- `const` correctness
- `auto` for complex types
- Range-based for loops

**Example**:
```cpp
// ❌ Bad
void update(Registry* reg, float dt) {
    for (int i = 0; i < reg->entities().size(); i++) {
        if (reg->has<Transform>(reg->entities()[i])) {
            Transform* t = &reg->get<Transform>(reg->entities()[i]);
            if (t->x > 1000) {
                t->x = 0;
            }
        }
    }
}

// ✅ Good
void update(Registry& registry, float dt) {
    constexpr float WRAP_BOUNDARY = 1000.f;
    
    registry.each<Transform>([](EntityID, Transform& t) {
        if (t.x > WRAP_BOUNDARY) {
            t.x = 0.f;
        }
    });
}
```

## Git Workflow

### Branch Naming
```
feat/RTYPE-123-add-boss-enemy     # New feature
fix/RTYPE-456-collision-bug       # Bug fix
docs/update-build-instructions    # Documentation
test/ecs-performance-benchmark    # Tests
refactor/simplify-movement-system # Refactoring
```

### Commit Messages

Format: `<type>(<scope>): <subject>`

**Types**: `feat`, `fix`, `docs`, `test`, `refactor`, `perf`, `style`

**Examples**:
```bash
feat(gameplay): add homing missile power-up
fix(server): resolve race condition in entity destruction
docs(readme): add macOS build instructions
test(ecs): add SparseSet removal edge cases
refactor(collision): extract AABB intersection logic
perf(render): batch sprite draw calls
```

**Rules**:
- Use imperative mood: "add" not "added"
- No period at end
- Keep under 72 characters
- Body explains "why", not "what" (code shows "what")

**Good commit body**:
```
fix(collision): prevent projectile double-hits

Projectiles were hitting multiple enemies per frame due to
collision system not marking hit projectiles for immediate
removal. Changed to mark projectiles during collision pass,
cleanup after all checks complete.

Fixes #123
```

## Testing Requirements

### Unit Tests Required
All new code **must** have tests:

```cpp
// tests/test_my_feature.cpp
#include <gtest/gtest.h>
#include <engine/systems/MySystem.hpp>

TEST(MySystemTest, DoesExpectedBehavior) {
    // Arrange
    Registry registry;
    MySystem system;
    EntityID entity = registry.create();
    registry.add<MyComponent>(entity, 42);
    
    // Act
    system.update(registry, 0.1f);
    
    // Assert
    EXPECT_EQ(registry.get<MyComponent>(entity).value, 43);
}

TEST(MySystemTest, HandlesEdgeCase) {
    // Test edge cases, null inputs, etc.
}
```

### Running Tests
```bash
cd build
./tests/rtype_tests
./tests/rtype_tests --gtest_filter=MySystemTest.*  # Run specific tests
```

### Manual Testing
Describe how to manually test your feature:
```markdown
## Testing Steps
1. Start server: `./r-type_server`
2. Start client: `./r-type-client`
3. Connect to localhost:4242
4. Press 'X' to activate new feature
5. Verify: Boss enemy spawns after 60 seconds
```

## Pull Request Process

### Before Submitting
- [ ] Code compiles without warnings
- [ ] All tests pass (`./rtype_tests`)
- [ ] New tests added for new code
- [ ] Code follows style guidelines
- [ ] No commented-out code or debug prints
- [ ] Documentation updated if needed

### PR Template
```markdown
## Description
Brief description of changes.

## Related Issue
Closes #123

## Type of Change
- [ ] Bug fix (non-breaking)
- [ ] New feature (non-breaking)
- [ ] Breaking change
- [ ] Documentation update

## How Has This Been Tested?
- [ ] Unit tests (list added tests)
- [ ] Manual testing (describe steps)
- [ ] Regression testing

## Checklist
- [ ] My code follows project style
- [ ] I've commented complex sections
- [ ] I've updated documentation
- [ ] My changes generate no new warnings
- [ ] I've added tests that prove my fix/feature works
- [ ] New and existing tests pass locally
```

### Review Process

1. **Automated Checks**: CI must pass (builds, tests, linting)
2. **Code Review**: At least 1 maintainer approval required
3. **Changes Requested**: Address feedback, push updates
4. **Approved**: Maintainer merges to `develop`

**Timeline**: Expect initial review within 2-3 business days

## Reporting Issues

### Bug Reports

Use this template:
```markdown
**Describe the bug**
Clear description of the issue.

**To Reproduce**
1. Start server
2. Connect 4 players
3. All players press space simultaneously
4. Crash occurs

**Expected behavior**
All players should shoot without issue.

**Environment**
- OS: Ubuntu 22.04
- Compiler: GCC 11.4
- Build type: Release
- Commit: abc1234

**Logs/Stack trace**
```
[paste error output]
```

**Additional context**
Happens only when >3 players shoot at exact same frame.
```

### Feature Requests

Use this template:
```markdown
**Is your feature request related to a problem?**
I'm frustrated when [...]

**Describe the solution you'd like**
Add a charge meter UI showing current charge level.

**Describe alternatives you've considered**
- Audio cue when fully charged
- Color change on ship

**Additional context**
[mockups, diagrams]
```

## Code of Conduct

### Our Pledge
We are committed to providing a welcoming, inclusive environment for all contributors.

### Expected Behavior
- Use welcoming and inclusive language
- Be respectful of differing viewpoints
- Accept constructive criticism gracefully
- Focus on what's best for the project
- Show empathy towards other contributors

### Unacceptable Behavior
- Harassment, discriminatory comments, or personal attacks
- Trolling, insulting/derogatory comments
- Public or private harassment
- Publishing others' private information
- Other conduct considered inappropriate in a professional setting

### Enforcement
Violations may result in:
1. Warning
2. Temporary ban from project
3. Permanent ban from project

Report violations to: [maintainer-email]

## Areas Needing Help

### High Priority
- [ ] Performance optimization in CollisionSystem
- [ ] Network packet compression
- [ ] Sound effects system
- [ ] Gamepad input support

### Good First Issues
Look for issues tagged `good-first-issue`:
- Documentation improvements
- Simple bug fixes
- Test coverage increases
- Asset creation (sprites, sounds)

### Long-Term Goals
- Port to Vulkan/OpenGL
- Add replay system
- Implement AI director
- Create level editor

## Getting Help

### Communication Channels
- **GitHub Issues**: Bug reports, feature requests
- **Discussions**: Design questions, brainstorming
- **Discord** (if available): Real-time chat

### Documentation
- `docs/USER_GUIDE.md` - Using the engine
- `docs/DEVELOPER_GUIDE.md` - Architecture deep-dive
- `docs/ECS_IMPLEMENTATION.md` - ECS technical details
- `docs/BENCHMARK.md` - Performance expectations

### Response Times
- Issues: 2-3 business days for first response
- PRs: 2-3 business days for initial review
- Critical bugs: 24 hours

## Recognition

Contributors are recognized in:
- `CONTRIBUTORS.md` (alphabetical list)
- Release notes for major contributions
- GitHub "Contributors" page

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (check `LICENSE` file).

---

**Thank you for contributing to R-Type!** 🚀

Questions? Open a [Discussion](https://github.com/YOUR_ORG/r-type/discussions) or reach out to maintainers.