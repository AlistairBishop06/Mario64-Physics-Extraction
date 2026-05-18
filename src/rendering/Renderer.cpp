#include "rendering/Renderer.h"

#include <SDL_opengl.h>

#include <glm/geometric.hpp>

namespace sm64ps::rendering {

namespace {

void setColor(const glm::vec3& color)
{
    glColor3f(color.x, color.y, color.z);
}

void vertex(const glm::vec3& value)
{
    glVertex3f(value.x, value.y, value.z);
}

} // namespace

bool Renderer::initialize(SDL_Window* window)
{
    window_ = window;
    context_ = SDL_GL_CreateContext(window);
    if (!context_) {
        return false;
    }

    SDL_GL_SetSwapInterval(1);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    return true;
}

void Renderer::shutdown()
{
    if (context_) {
        SDL_GL_DeleteContext(context_);
        context_ = nullptr;
    }
}

void Renderer::beginFrame(int width, int height)
{
    glViewport(0, 0, width, height);
    glClearColor(0.08f, 0.09f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    glOrtho(-145.0f * aspect, 145.0f * aspect, -145.0f, 145.0f, -220.0f, 220.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
}

void Renderer::drawCollision(const collision::CollisionWorld& collisionWorld)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_TRIANGLES);
    for (const auto& triangle : collisionWorld.triangles()) {
        setColor(triangle.normal.y > 0.8f ? glm::vec3(0.35f, 0.75f, 0.40f) : glm::vec3(0.65f, 0.65f, 0.75f));
        vertex(triangle.a);
        vertex(triangle.b);
        vertex(triangle.c);
    }
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::drawMario(const mario::MarioBody& body)
{
    const glm::vec3 p = body.position;
    const float r = 3.5f;
    const float h = 16.0f;

    glBegin(GL_LINES);
    setColor({ 1.0f, 0.15f, 0.12f });
    vertex({ p.x - r, p.y, p.z });
    vertex({ p.x + r, p.y, p.z });
    vertex({ p.x, p.y, p.z - r });
    vertex({ p.x, p.y, p.z + r });
    vertex({ p.x, p.y, p.z });
    vertex({ p.x, p.y + h, p.z });
    glEnd();
}

void Renderer::drawDebugLines(const DebugRenderer& debugRenderer)
{
    glBegin(GL_LINES);
    for (const auto& line : debugRenderer.lines()) {
        setColor(line.color);
        vertex(line.a);
        vertex(line.b);
    }
    glEnd();
}

void Renderer::endFrame()
{
    SDL_GL_SwapWindow(window_);
}

} // namespace sm64ps::rendering
