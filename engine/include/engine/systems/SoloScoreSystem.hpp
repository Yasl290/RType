#pragma once

#include <engine/core/System.hpp>
#include <engine/core/Registry.hpp>

class SoloScoreSystem : public System {
public:
    SoloScoreSystem();
    
    void update(Registry& registry, float dt) override;
    void render(Registry&, Renderer&) override {}
};