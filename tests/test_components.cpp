#include <gtest/gtest.h>
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Health.hpp"
#include "engine/gameplay/Controllable.hpp"
#include "engine/gameplay/Projectile.hpp"
#include "engine/gameplay/Score.hpp"

TEST(TransformTest, DefaultConstructor) {
    Transform t;
    EXPECT_FLOAT_EQ(t.x, 0.f);
    EXPECT_FLOAT_EQ(t.y, 0.f);
    EXPECT_FLOAT_EQ(t.rotation, 0.f);
    EXPECT_FLOAT_EQ(t.scaleX, 1.f);
    EXPECT_FLOAT_EQ(t.scaleY, 1.f);
}

TEST(TransformTest, ParameterizedConstructor) {
    Transform t(10.f, 20.f);
    EXPECT_FLOAT_EQ(t.x, 10.f);
    EXPECT_FLOAT_EQ(t.y, 20.f);
    EXPECT_FLOAT_EQ(t.rotation, 0.f);
    EXPECT_FLOAT_EQ(t.scaleX, 1.f);
    EXPECT_FLOAT_EQ(t.scaleY, 1.f);
}

TEST(VelocityTest, DefaultConstructor) {
    Velocity v;
    EXPECT_FLOAT_EQ(v.x, 0.f);
    EXPECT_FLOAT_EQ(v.y, 0.f);
}

TEST(VelocityTest, ParameterizedConstructor) {
    Velocity v(5.f, 10.f);
    EXPECT_FLOAT_EQ(v.x, 5.f);
    EXPECT_FLOAT_EQ(v.y, 10.f);
}

TEST(HealthTest, DefaultConstructor) {
    Health h;
    EXPECT_FLOAT_EQ(h.current, 100.f);
    EXPECT_FLOAT_EQ(h.max, 100.f);
}

TEST(HealthTest, ParameterizedConstructor) {
    Health h(50.f);
    EXPECT_FLOAT_EQ(h.current, 50.f);
    EXPECT_FLOAT_EQ(h.max, 50.f);
}

TEST(ControllableTest, DefaultConstructor) {
    Controllable c;
    EXPECT_FLOAT_EQ(c.speed, 300.f);
    EXPECT_TRUE(c.canShoot);
    EXPECT_FLOAT_EQ(c.shootCooldown, 0.2f);
    EXPECT_FLOAT_EQ(c.currentCooldown, 0.f);
}

TEST(ControllableTest, ParameterizedConstructor) {
    Controllable c(500.f);
    EXPECT_FLOAT_EQ(c.speed, 500.f);
}

TEST(ProjectileTest, DefaultConstructor) {
    Projectile p;
    EXPECT_EQ(p.type, ProjectileType::Normal);
    EXPECT_FLOAT_EQ(p.damage, 10.f);
    EXPECT_FALSE(p.piercing);
    EXPECT_TRUE(p.isPlayerProjectile);
}

TEST(ProjectileTest, ChargedProjectile) {
    Projectile p(ProjectileType::Charged, 50.f, true, 1);
    EXPECT_EQ(p.type, ProjectileType::Charged);
    EXPECT_FLOAT_EQ(p.damage, 50.f);
    EXPECT_TRUE(p.piercing);
    EXPECT_EQ(p.ownerId, 1);
}

TEST(ScoreTest, DefaultConstructor) {
    Score s;
    EXPECT_EQ(s.getPoints(), 0);
    EXPECT_EQ(s.getEnemiesKilled(), 0);
}

TEST(ScoreTest, AddPoints) {
    Score s;
    s.addPoints(100);
    EXPECT_EQ(s.getPoints(), 100);
    
    s.addPoints(50);
    EXPECT_EQ(s.getPoints(), 150);
}

TEST(ScoreTest, IncrementKills) {
    Score s;
    s.incrementKills();
    EXPECT_EQ(s.getEnemiesKilled(), 1);
    
    s.incrementKills();
    EXPECT_EQ(s.getEnemiesKilled(), 2);
}

TEST(ScoreTest, Reset) {
    Score s;
    s.addPoints(100);
    s.incrementKills();
    s.incrementKills();
    
    EXPECT_EQ(s.getPoints(), 100);
    EXPECT_EQ(s.getEnemiesKilled(), 2);
    
    s.reset();
    
    EXPECT_EQ(s.getPoints(), 0);
    EXPECT_EQ(s.getEnemiesKilled(), 0);
}