#pragma once

class Registry;
class Renderer;

class System {
public:
    virtual ~System() = default;
    virtual void update(Registry& registry, float deltaTime) = 0;
    virtual void render(Registry& registry, Renderer& renderer);
};