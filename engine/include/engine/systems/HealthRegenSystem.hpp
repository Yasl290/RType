#pragma once

#include <engine/core/System.hpp>
#include <engine/core/Registry.hpp>

class HealthRegenSystem : public System {
public:
    HealthRegenSystem();
    
    void update(Registry& registry, float dt) override;
    void render(Registry&, Renderer&) override {}

private:
    float _regenTimer;
};