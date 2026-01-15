#pragma once

#include "engine/core/Component.hpp"

enum class EnemyType {
    Basic = 0
};

class Enemy : public Component {
public:
    Enemy();
    explicit Enemy(EnemyType type);
    ~Enemy() override = default;

    EnemyType type;
};