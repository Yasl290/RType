#include <gtest/gtest.h>
#include "engine/core/Registry.hpp"
#include "engine/systems/MovementSystem.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"

class MovementSystemTest : public ::testing::Test {
protected:
    Registry registry;
    MovementSystem movementSystem;
};

TEST_F(MovementSystemTest, UpdatePosition) {
    EntityID entity = registry.create();
    registry.add<Transform>(entity, 0.f, 0.f);
    registry.add<Velocity>(entity, 10.f, 5.f);
    
    movementSystem.update(registry, 1.0f);
    
    Transform& t = registry.get<Transform>(entity);
    EXPECT_FLOAT_EQ(t.x, 10.f);
    EXPECT_FLOAT_EQ(t.y, 5.f);
}

TEST_F(MovementSystemTest, UpdatePositionWithDeltaTime) {
    EntityID entity = registry.create();
    registry.add<Transform>(entity, 0.f, 0.f);
    registry.add<Velocity>(entity, 100.f, 50.f);
    
    movementSystem.update(registry, 0.5f);
    
    Transform& t = registry.get<Transform>(entity);
    EXPECT_FLOAT_EQ(t.x, 50.f);
    EXPECT_FLOAT_EQ(t.y, 25.f);
}

TEST_F(MovementSystemTest, MultipleEntities) {
    EntityID e1 = registry.create();
    EntityID e2 = registry.create();
    
    registry.add<Transform>(e1, 0.f, 0.f);
    registry.add<Velocity>(e1, 10.f, 10.f);
    
    registry.add<Transform>(e2, 100.f, 100.f);
    registry.add<Velocity>(e2, -5.f, -5.f);
    
    movementSystem.update(registry, 1.0f);
    
    Transform& t1 = registry.get<Transform>(e1);
    Transform& t2 = registry.get<Transform>(e2);
    
    EXPECT_FLOAT_EQ(t1.x, 10.f);
    EXPECT_FLOAT_EQ(t1.y, 10.f);
    EXPECT_FLOAT_EQ(t2.x, 95.f);
    EXPECT_FLOAT_EQ(t2.y, 95.f);
}

TEST_F(MovementSystemTest, NoVelocityComponent) {
    EntityID entity = registry.create();
    registry.add<Transform>(entity, 10.f, 10.f);
    
    movementSystem.update(registry, 1.0f);
    
    Transform& t = registry.get<Transform>(entity);
    EXPECT_FLOAT_EQ(t.x, 10.f);
    EXPECT_FLOAT_EQ(t.y, 10.f);
}

TEST_F(MovementSystemTest, ZeroVelocity) {
    EntityID entity = registry.create();
    registry.add<Transform>(entity, 10.f, 10.f);
    registry.add<Velocity>(entity, 0.f, 0.f);
    
    movementSystem.update(registry, 1.0f);
    
    Transform& t = registry.get<Transform>(entity);
    EXPECT_FLOAT_EQ(t.x, 10.f);
    EXPECT_FLOAT_EQ(t.y, 10.f);
}