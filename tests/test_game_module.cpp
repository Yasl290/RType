#include <gtest/gtest.h>
#include "GameModule.hpp"

class GameModuleTest : public ::testing::Test {
protected:
    GameModule gameModule;
    
    void SetUp() override {
        gameModule.init();
    }
};

TEST_F(GameModuleTest, InitializeModule) {
    EXPECT_NO_THROW(gameModule.init());
}

TEST_F(GameModuleTest, SpawnPlayer) {
    gameModule.spawnPlayer(1, 100.f, 200.f, 0);
    auto snapshot = gameModule.getWorldSnapshot();
    EXPECT_GT(snapshot.size(), 0);
}

TEST_F(GameModuleTest, SpawnMultiplePlayers) {
    uint32_t p1 = gameModule.spawnPlayer(1, 100.f, 200.f, 0);
    uint32_t p2 = gameModule.spawnPlayer(2, 150.f, 250.f, 1);
    uint32_t p3 = gameModule.spawnPlayer(3, 200.f, 300.f, 2);
    
    EXPECT_NE(p1, p2);
    EXPECT_NE(p2, p3);
    EXPECT_NE(p1, p3);
    
    auto snapshot = gameModule.getWorldSnapshot();
    EXPECT_EQ(snapshot.size(), 3);
}

TEST_F(GameModuleTest, RemovePlayer) {
    gameModule.spawnPlayer(1, 100.f, 200.f, 0);
    EXPECT_NO_THROW(gameModule.removePlayer(1));
}

TEST_F(GameModuleTest, ProcessPlayerInput) {
    gameModule.spawnPlayer(1, 100.f, 200.f, 0);
    
    uint8_t moveRight = 0x08;
    EXPECT_NO_THROW(gameModule.processInput(1, moveRight));
}

TEST_F(GameModuleTest, UpdateModule) {
    gameModule.spawnPlayer(1, 100.f, 200.f, 0);
    
    EXPECT_NO_THROW(gameModule.update(0.016f));
}

TEST_F(GameModuleTest, GetWorldSnapshot) {
    gameModule.spawnPlayer(1, 100.f, 200.f, 0);
    
    auto snapshot = gameModule.getWorldSnapshot();
    EXPECT_GT(snapshot.size(), 0);
}

TEST_F(GameModuleTest, PlayerScore) {
    gameModule.spawnPlayer(1, 100.f, 200.f, 0);
    
    uint32_t score = gameModule.getPlayerScore(1);
    EXPECT_EQ(score, 0);
}

TEST_F(GameModuleTest, PlayerMovement) {
    gameModule.spawnPlayer(1, 100.f, 200.f, 0);
    
    uint8_t moveRight = 0x08;
    gameModule.processInput(1, moveRight);
    
    auto snapshotBefore = gameModule.getWorldSnapshot();
    float xBefore = snapshotBefore[0].pos_x;
    
    gameModule.update(1.0f);
    
    auto snapshotAfter = gameModule.getWorldSnapshot();
    float xAfter = snapshotAfter[0].pos_x;
    
    EXPECT_GT(xAfter, xBefore);
}

TEST_F(GameModuleTest, MaxPlayersLimit) {
    gameModule.spawnPlayer(1, 100.f, 200.f, 0);
    gameModule.spawnPlayer(2, 150.f, 250.f, 1);
    gameModule.spawnPlayer(3, 200.f, 300.f, 2);
    gameModule.spawnPlayer(4, 250.f, 350.f, 3);
    
    auto snapshot = gameModule.getWorldSnapshot();
    EXPECT_EQ(snapshot.size(), 4);
    
    uint32_t p5 = gameModule.spawnPlayer(5, 300.f, 400.f, 0);
    EXPECT_EQ(p5, 0);
    
    snapshot = gameModule.getWorldSnapshot();
    EXPECT_EQ(snapshot.size(), 4);
}

TEST_F(GameModuleTest, ShootProjectile) {
    gameModule.spawnPlayer(1, 100.f, 200.f, 0);
    
    uint8_t shoot = 0x10;
    gameModule.processInput(1, shoot);
    gameModule.update(0.016f);
    
    gameModule.processInput(1, 0);
    gameModule.update(0.016f);
    
    auto snapshot = gameModule.getWorldSnapshot();
    bool hasProjectile = false;
    for (const auto& entity : snapshot) {
        if (entity.entity_type == 2 || entity.entity_type == 3) {
            hasProjectile = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasProjectile);
}