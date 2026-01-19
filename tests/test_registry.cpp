#include <gtest/gtest.h>
#include "engine/core/Registry.hpp"
#include "engine/physics/Transform.hpp"
#include "engine/physics/Velocity.hpp"
#include "engine/gameplay/Health.hpp"

class RegistryTest : public ::testing::Test {
protected:
    Registry registry;
};

TEST_F(RegistryTest, CreateEntity) {
    EntityID entity = registry.create();
    EXPECT_GE(entity, 0);
}

TEST_F(RegistryTest, CreateMultipleEntities) {
    EntityID e1 = registry.create();
    EntityID e2 = registry.create();
    EntityID e3 = registry.create();
    
    EXPECT_NE(e1, e2);
    EXPECT_NE(e2, e3);
    EXPECT_NE(e1, e3);
}

TEST_F(RegistryTest, AddComponent) {
    EntityID entity = registry.create();
    Transform& transform = registry.add<Transform>(entity, 10.f, 20.f);
    
    EXPECT_FLOAT_EQ(transform.x, 10.f);
    EXPECT_FLOAT_EQ(transform.y, 20.f);
}

TEST_F(RegistryTest, GetComponent) {
    EntityID entity = registry.create();
    registry.add<Transform>(entity, 10.f, 20.f);
    
    Transform& transform = registry.get<Transform>(entity);
    EXPECT_FLOAT_EQ(transform.x, 10.f);
    EXPECT_FLOAT_EQ(transform.y, 20.f);
}

TEST_F(RegistryTest, HasComponent) {
    EntityID entity = registry.create();
    EXPECT_FALSE(registry.has<Transform>(entity));
    
    registry.add<Transform>(entity, 10.f, 20.f);
    EXPECT_TRUE(registry.has<Transform>(entity));
}

TEST_F(RegistryTest, MultipleComponents) {
    EntityID entity = registry.create();
    
    registry.add<Transform>(entity, 10.f, 20.f);
    registry.add<Velocity>(entity, 5.f, 3.f);
    registry.add<Health>(entity, 100.f);
    
    EXPECT_TRUE(registry.has<Transform>(entity));
    EXPECT_TRUE(registry.has<Velocity>(entity));
    EXPECT_TRUE(registry.has<Health>(entity));
    
    EXPECT_FLOAT_EQ(registry.get<Transform>(entity).x, 10.f);
    EXPECT_FLOAT_EQ(registry.get<Velocity>(entity).x, 5.f);
    EXPECT_FLOAT_EQ(registry.get<Health>(entity).current, 100.f);
}

TEST_F(RegistryTest, IterateEntitiesWithComponents) {
    EntityID e1 = registry.create();
    EntityID e2 = registry.create();
    EntityID e3 = registry.create();
    
    registry.add<Transform>(e1, 1.f, 1.f);
    registry.add<Transform>(e2, 2.f, 2.f);
    registry.add<Transform>(e3, 3.f, 3.f);
    
    registry.add<Velocity>(e1, 10.f, 10.f);
    registry.add<Velocity>(e2, 20.f, 20.f);
    
    int count = 0;
    registry.each<Transform, Velocity>([&count](EntityID, Transform&, Velocity&) {
        count++;
    });
    
    EXPECT_EQ(count, 2);
}

TEST_F(RegistryTest, DestroyEntity) {
    EntityID entity = registry.create();
    registry.add<Transform>(entity, 10.f, 20.f);
    
    EXPECT_TRUE(registry.has<Transform>(entity));
    
    registry.destroy(entity);
    
    EXPECT_FALSE(registry.has<Transform>(entity));
}

TEST_F(RegistryTest, MarkForDestruction) {
    EntityID entity = registry.create();
    registry.add<Transform>(entity, 10.f, 20.f);
    
    registry.markForDestruction(entity);
    EXPECT_TRUE(registry.has<Transform>(entity));
    
    registry.cleanup();
    EXPECT_FALSE(registry.has<Transform>(entity));
}

TEST_F(RegistryTest, ModifyComponentDuringIteration) {
    EntityID e1 = registry.create();
    EntityID e2 = registry.create();
    
    registry.add<Transform>(e1, 1.f, 1.f);
    registry.add<Transform>(e2, 2.f, 2.f);
    
    registry.each<Transform>([](EntityID, Transform& t) {
        t.x += 10.f;
        t.y += 10.f;
    });
    
    EXPECT_FLOAT_EQ(registry.get<Transform>(e1).x, 11.f);
    EXPECT_FLOAT_EQ(registry.get<Transform>(e1).y, 11.f);
    EXPECT_FLOAT_EQ(registry.get<Transform>(e2).x, 12.f);
    EXPECT_FLOAT_EQ(registry.get<Transform>(e2).y, 12.f);
}