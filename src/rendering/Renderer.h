#pragma once

#include "collision/CollisionWorld.h"
#include "mario/MarioState.h"
#include "rendering/DebugRenderer.h"

#include <SDL.h>

namespace sm64ps::rendering {

class Renderer {
public:
    bool initialize(SDL_Window* window);
    void shutdown();
    void beginFrame(int width, int height);
    void drawCollision(const collision::CollisionWorld& collisionWorld);
    void drawMario(const mario::MarioBody& body);
    void drawDebugLines(const DebugRenderer& debugRenderer);
    void endFrame();

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
};

} // namespace sm64ps::rendering

