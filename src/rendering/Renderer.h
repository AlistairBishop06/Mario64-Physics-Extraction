#pragma once

#include "assets/RuntimeAssets.h"
#include "collision/CollisionWorld.h"
#include "mario/MarioState.h"
#include "rendering/DebugRenderer.h"

#include <SDL.h>

namespace sm64ps::rendering {

class Renderer {
public:
    bool initialize(SDL_Window* window);
    void shutdown();
    void setMarioTexture(const std::vector<std::uint8_t>& rgba, int width, int height);
    void updateLakituCamera(const mario::MarioBody& body, float orbitInput, float zoomInput);
    glm::vec3 cameraLookDirection() const;
    void beginFrame(int width, int height);
    void drawCollision(const collision::CollisionWorld& collisionWorld);
    void drawMario(const mario::MarioBody& body, const assets::Mesh* mesh = nullptr);
    void drawDebugLines(const DebugRenderer& debugRenderer);
    void endFrame();

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    unsigned int marioTexture_ = 0;
    glm::vec3 cameraTarget_ { 0.0f, 14.0f, 0.0f };
    glm::vec3 cameraPosition_ { 0.0f, 260.0f, -900.0f };
    float cameraYaw_ = 3.14159265f;
    float cameraDistance_ = 1050.0f;
    bool cameraInitialized_ = false;
};

} // namespace sm64ps::rendering
