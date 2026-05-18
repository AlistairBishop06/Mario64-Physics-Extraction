#include "rendering/Renderer.h"

#include "mario/MarioMath.h"

#include <SDL_opengl.h>

#include <algorithm>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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

void drawBox(const glm::vec3& center, const glm::vec3& halfExtents, const glm::vec3& color)
{
    const glm::vec3 p0 = center + glm::vec3(-halfExtents.x, -halfExtents.y, -halfExtents.z);
    const glm::vec3 p1 = center + glm::vec3(halfExtents.x, -halfExtents.y, -halfExtents.z);
    const glm::vec3 p2 = center + glm::vec3(halfExtents.x, halfExtents.y, -halfExtents.z);
    const glm::vec3 p3 = center + glm::vec3(-halfExtents.x, halfExtents.y, -halfExtents.z);
    const glm::vec3 p4 = center + glm::vec3(-halfExtents.x, -halfExtents.y, halfExtents.z);
    const glm::vec3 p5 = center + glm::vec3(halfExtents.x, -halfExtents.y, halfExtents.z);
    const glm::vec3 p6 = center + glm::vec3(halfExtents.x, halfExtents.y, halfExtents.z);
    const glm::vec3 p7 = center + glm::vec3(-halfExtents.x, halfExtents.y, halfExtents.z);

    setColor(color);
    glBegin(GL_QUADS);
    vertex(p0); vertex(p1); vertex(p2); vertex(p3);
    vertex(p5); vertex(p4); vertex(p7); vertex(p6);
    vertex(p4); vertex(p0); vertex(p3); vertex(p7);
    vertex(p1); vertex(p5); vertex(p6); vertex(p2);
    vertex(p3); vertex(p2); vertex(p6); vertex(p7);
    vertex(p4); vertex(p5); vertex(p1); vertex(p0);
    glEnd();
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
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    return true;
}

void Renderer::shutdown()
{
    if (marioTexture_ != 0) {
        glDeleteTextures(1, &marioTexture_);
        marioTexture_ = 0;
    }
    if (context_) {
        SDL_GL_DeleteContext(context_);
        context_ = nullptr;
    }
}

void Renderer::setMarioTexture(const std::vector<std::uint8_t>& rgba, int width, int height)
{
    if (rgba.empty() || width <= 0 || height <= 0) {
        return;
    }
    if (marioTexture_ == 0) {
        glGenTextures(1, &marioTexture_);
    }
    glBindTexture(GL_TEXTURE_2D, marioTexture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::followMarioCamera(const mario::MarioBody& body)
{
    const glm::vec3 forward = mario::yawToForward(body.faceYaw);
    const glm::vec3 desiredTarget = body.position + glm::vec3(0.0f, 105.0f, 0.0f);
    const glm::vec3 desiredPosition = desiredTarget - forward * 850.0f + glm::vec3(0.0f, 260.0f, 0.0f);

    if (!cameraInitialized_) {
        cameraTarget_ = desiredTarget;
        cameraPosition_ = desiredPosition;
        cameraInitialized_ = true;
        return;
    }

    cameraTarget_ += (desiredTarget - cameraTarget_) * 0.18f;
    cameraPosition_ += (desiredPosition - cameraPosition_) * 0.10f;
}

void Renderer::beginFrame(int width, int height)
{
    glViewport(0, 0, width, height);
    glClearColor(0.12f, 0.15f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    const glm::mat4 projection = glm::perspective(glm::radians(50.0f), aspect, 5.0f, 8000.0f);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
    const glm::mat4 view = glm::lookAt(
        cameraPosition_,
        cameraTarget_,
        glm::vec3(0.0f, 1.0f, 0.0f));
    glLoadMatrixf(glm::value_ptr(view));
}

void Renderer::drawCollision(const collision::CollisionWorld& collisionWorld)
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    for (const auto& triangle : collisionWorld.triangles()) {
        if (triangle.normal.y > 0.8f) {
            setColor({ 0.18f, 0.42f, 0.26f });
        } else if (triangle.normal.y > 0.2f) {
            setColor({ 0.36f, 0.36f, 0.40f });
        } else {
            setColor({ 0.28f, 0.31f, 0.37f });
        }
        vertex(triangle.a);
        vertex(triangle.b);
        vertex(triangle.c);
    }
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);

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

void Renderer::drawMario(const mario::MarioBody& body, const assets::Mesh* mesh)
{
    const glm::vec3 p = body.position;

    glPushMatrix();
    if (!mesh || !mesh->worldSpace) {
        glTranslatef(p.x, p.y, p.z);
        glRotatef(static_cast<float>(body.faceYaw) * 360.0f / 65536.0f, 0.0f, 1.0f, 0.0f);
    }

    if (mesh && !mesh->empty()) {
        if (marioTexture_ != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, marioTexture_);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        }
        glBegin(GL_TRIANGLES);
        for (const auto& meshVertex : mesh->vertices) {
            const float light = 0.45f + 0.55f * std::max(meshVertex.normal.y, 0.0f);
            glColor4f(meshVertex.color.r * light, meshVertex.color.g * light, meshVertex.color.b * light, meshVertex.color.a);
            glTexCoord2f(meshVertex.uv.x, meshVertex.uv.y);
            vertex(meshVertex.position);
        }
        glEnd();
        if (marioTexture_ != 0) {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
        }
        glPopMatrix();
        return;
    }

    drawBox({ 0.0f, 24.0f, 0.0f }, { 4.0f, 4.0f, 4.0f }, { 0.95f, 0.72f, 0.55f });
    drawBox({ 0.0f, 30.0f, -0.5f }, { 5.0f, 2.0f, 4.5f }, { 0.78f, 0.05f, 0.04f });
    drawBox({ 0.0f, 16.0f, 0.0f }, { 5.0f, 7.0f, 3.0f }, { 0.10f, 0.22f, 0.78f });
    drawBox({ -4.5f, 17.0f, 0.0f }, { 1.5f, 5.0f, 1.5f }, { 0.78f, 0.05f, 0.04f });
    drawBox({ 4.5f, 17.0f, 0.0f }, { 1.5f, 5.0f, 1.5f }, { 0.78f, 0.05f, 0.04f });
    drawBox({ -2.2f, 7.0f, 0.0f }, { 1.6f, 7.0f, 1.7f }, { 0.10f, 0.22f, 0.78f });
    drawBox({ 2.2f, 7.0f, 0.0f }, { 1.6f, 7.0f, 1.7f }, { 0.10f, 0.22f, 0.78f });
    drawBox({ -2.2f, 1.0f, 1.5f }, { 2.4f, 1.2f, 3.5f }, { 0.33f, 0.15f, 0.05f });
    drawBox({ 2.2f, 1.0f, 1.5f }, { 2.4f, 1.2f, 3.5f }, { 0.33f, 0.15f, 0.05f });

    glBegin(GL_LINES);
    setColor({ 1.0f, 1.0f, 1.0f });
    vertex({ 0.0f, 18.0f, 0.0f });
    vertex({ 0.0f, 18.0f, 12.0f });
    glEnd();
    glPopMatrix();
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
