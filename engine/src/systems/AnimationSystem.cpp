#include "engine/systems/AnimationSystem.hpp"
#include "engine/core/Registry.hpp"
#include "engine/graphics/Animation.hpp"
#include "engine/graphics/Sprite.hpp"
#include <SFML/Graphics/Rect.hpp>

AnimationSystem::AnimationSystem() = default;

void AnimationSystem::update(Registry& registry, float deltaTime)
{
    registry.each<Animation, Sprite>([deltaTime](EntityID, Animation& a, Sprite& s) {
        if (a.frameCount == 0)
            return;

        a.elapsedTime += deltaTime;

        if (a.elapsedTime >= a.frameTime) {
            a.elapsedTime = 0.f;
            a.currentFrame++;

            if (a.currentFrame >= a.frameCount) {
                if (a.loop) {
                    a.currentFrame = 0;
                } else {
                    a.currentFrame = a.frameCount - 1;
                }
            }

            const AnimationFrame* frame = a.getCurrentFrame();
            if (frame) {
                s.getSprite().setTextureRect(
                    sf::IntRect(frame->x, frame->y, frame->width, frame->height)
                );
            }
        }
    });
}