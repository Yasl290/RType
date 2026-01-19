add_library(engine STATIC
    src/core/Registry.cpp
    src/core/System.cpp
    src/graphics/Renderer.cpp
    src/graphics/Sprite.cpp
    src/graphics/Animation.cpp
    src/graphics/SpriteSheet.cpp
    src/physics/Transform.cpp
    src/physics/Velocity.cpp
    src/gameplay/Controllable.cpp
    src/gameplay/Projectile.cpp
    src/gameplay/Health.cpp
    src/gameplay/Enemy.cpp
    src/gameplay/EnemyFactory.cpp
    src/gameplay/EnemyShooter.cpp
    src/gameplay/Score.cpp
    src/systems/RenderSystem.cpp
    src/systems/MovementSystem.cpp
    src/systems/InputSystem.cpp
    src/systems/BoundarySystem.cpp
    src/systems/ScrollingBackgroundSystem.cpp
    src/systems/AnimationSystem.cpp
    src/systems/ShootingSystem.cpp
    src/systems/CleanupSystem.cpp
    src/audio/AudioManager.cpp
    src/systems/CollisionSystem.cpp
    src/systems/EnemySpawnSystem.cpp
    src/systems/EnemyShootingSystem.cpp
    src/systems/EnemyHealthBarSystem.cpp
    src/systems/PlayerHealthBarSystem.cpp
    src/systems/BossHealthBarSystem.cpp
    src/systems/ScoreDisplaySystem.cpp
    src/gameplay/UpgradeSelector.cpp
    src/systems/SoloScoreSystem.cpp
    src/systems/HealthRegenSystem.cpp
    src/gameplay/HighscoreManager.cpp
    src/systems/HighscoreSystem.cpp
    
)

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(engine PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
)
