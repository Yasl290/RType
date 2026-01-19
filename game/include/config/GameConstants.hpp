#pragma once

namespace GameConstants {
    // ====== Window and Display ======
    constexpr float WINDOW_WIDTH = 1280.f;
    constexpr float WINDOW_HEIGHT = 720.f;
    constexpr float LOGICAL_WIDTH = 1280.f;
    constexpr float LOGICAL_HEIGHT = 720.f;

    // ====== Network ======
    constexpr float DESPAWN_TIMEOUT = 0.1f;
    constexpr float SERVER_TIMEOUT = 3.0f;  // Server considered dead after 3 seconds
    constexpr float HEARTBEAT_INTERVAL = 1.0f;

    // ====== Player ======
    constexpr float PLAYER_SPAWN_X = 100.f;
    constexpr float PLAYER_SPAWN_Y_BASE = 150.f;
    constexpr float PLAYER_SPAWN_Y_SPACING = 120.f;
    constexpr float PLAYER_SCALE = 0.4f;
    constexpr float PLAYER_DEFAULT_HP = 100.f;

    // Player sprite texture rect
    constexpr int PLAYER_SPRITE_RECT_X = 0;
    constexpr int PLAYER_SPRITE_RECT_Y = 30;
    constexpr int PLAYER_SPRITE_RECT_W = 316;
    constexpr int PLAYER_SPRITE_RECT_H = 160;

    // ====== Background ======
    constexpr float BG_NATIVE_WIDTH = 1280.f;
    constexpr float BG_NATIVE_HEIGHT = 720.f;
    constexpr float BG_SCROLL_SPEED = -100.f;

    // ====== Enemy ======
    constexpr float ENEMY_SCALE = 0.6f;
    constexpr float ENEMY_DEFAULT_HP = 50.f;
    constexpr float ENEMY_PROJECTILE_SCALE = 0.7f;

    // ====== Boss ======
    constexpr float BOSS_SCALE = 3.0f;
    constexpr float BOSS_DEFAULT_HP = 10000.f;
    constexpr float BOSS_SHOT_SCALE = 1.5f;
    constexpr float BOSS_SHOT_SPEED_THRESHOLD = 260.f;

    // ====== Animation ======
    constexpr float ANIMATION_FRAME_TIME = 0.2f;  // 5 FPS (200ms per frame)
    constexpr float BOSS_SHOT_FRAME_TIME = 0.1f;  // 10 FPS for boss projectiles

    // ====== Collision Boxes ======
    constexpr float ENEMY_WIDTH = 100.f;
    constexpr float ENEMY_HEIGHT = 60.f;
    constexpr float PLAYER_WIDTH = 100.f;
    constexpr float PLAYER_HEIGHT = 60.f;
    constexpr float BULLET_WIDTH = 20.f;
    constexpr float BULLET_HEIGHT = 20.f;
}