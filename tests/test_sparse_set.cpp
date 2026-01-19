#include <gtest/gtest.h>
#include "engine/core/SparseSet.hpp"
#include "engine/physics/Transform.hpp"

class SparseSetTest : public ::testing::Test {
protected:
    SparseSet<Transform> sparseSet;
};

TEST_F(SparseSetTest, AddElement) {
    EntityID entity = 42;
    Transform& t = sparseSet.add(entity, 10.f, 20.f);
    
    EXPECT_FLOAT_EQ(t.x, 10.f);
    EXPECT_FLOAT_EQ(t.y, 20.f);
}

TEST_F(SparseSetTest, ContainsElement) {
    EntityID entity = 42;
    
    EXPECT_FALSE(sparseSet.contains(entity));
    
    sparseSet.add(entity, 10.f, 20.f);
    
    EXPECT_TRUE(sparseSet.contains(entity));
}

TEST_F(SparseSetTest, GetElement) {
    EntityID entity = 42;
    sparseSet.add(entity, 10.f, 20.f);
    
    Transform& t = sparseSet.get(entity);
    EXPECT_FLOAT_EQ(t.x, 10.f);
    EXPECT_FLOAT_EQ(t.y, 20.f);
}

TEST_F(SparseSetTest, ModifyElement) {
    EntityID entity = 42;
    sparseSet.add(entity, 10.f, 20.f);
    
    Transform& t = sparseSet.get(entity);
    t.x = 30.f;
    t.y = 40.f;
    
    Transform& t2 = sparseSet.get(entity);
    EXPECT_FLOAT_EQ(t2.x, 30.f);
    EXPECT_FLOAT_EQ(t2.y, 40.f);
}

TEST_F(SparseSetTest, RemoveElement) {
    EntityID entity = 42;
    sparseSet.add(entity, 10.f, 20.f);
    
    EXPECT_TRUE(sparseSet.contains(entity));
    
    sparseSet.remove(entity);
    
    EXPECT_FALSE(sparseSet.contains(entity));
}

TEST_F(SparseSetTest, RemoveNonExistentElement) {
    EntityID entity = 42;
    
    EXPECT_NO_THROW(sparseSet.remove(entity));
}

TEST_F(SparseSetTest, MultipleElements) {
    EntityID e1 = 1;
    EntityID e2 = 2;
    EntityID e3 = 3;
    
    sparseSet.add(e1, 1.f, 1.f);
    sparseSet.add(e2, 2.f, 2.f);
    sparseSet.add(e3, 3.f, 3.f);
    
    EXPECT_TRUE(sparseSet.contains(e1));
    EXPECT_TRUE(sparseSet.contains(e2));
    EXPECT_TRUE(sparseSet.contains(e3));
    
    EXPECT_FLOAT_EQ(sparseSet.get(e1).x, 1.f);
    EXPECT_FLOAT_EQ(sparseSet.get(e2).x, 2.f);
    EXPECT_FLOAT_EQ(sparseSet.get(e3).x, 3.f);
}

TEST_F(SparseSetTest, RemoveMiddleElement) {
    EntityID e1 = 1;
    EntityID e2 = 2;
    EntityID e3 = 3;
    
    sparseSet.add(e1, 1.f, 1.f);
    sparseSet.add(e2, 2.f, 2.f);
    sparseSet.add(e3, 3.f, 3.f);
    
    sparseSet.remove(e2);
    
    EXPECT_TRUE(sparseSet.contains(e1));
    EXPECT_FALSE(sparseSet.contains(e2));
    EXPECT_TRUE(sparseSet.contains(e3));
    
    EXPECT_FLOAT_EQ(sparseSet.get(e1).x, 1.f);
    EXPECT_FLOAT_EQ(sparseSet.get(e3).x, 3.f);
}

TEST_F(SparseSetTest, SparseIndices) {
    EntityID e1 = 5;
    EntityID e2 = 100;
    EntityID e3 = 1000;
    
    sparseSet.add(e1, 5.f, 5.f);
    sparseSet.add(e2, 100.f, 100.f);
    sparseSet.add(e3, 1000.f, 1000.f);
    
    EXPECT_TRUE(sparseSet.contains(e1));
    EXPECT_TRUE(sparseSet.contains(e2));
    EXPECT_TRUE(sparseSet.contains(e3));
}