#include "mario/MarioKartBackend.h"

#include "mario/MarioMath.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace sm64ps::mario {

namespace {

constexpr float kPi = 3.1415926535f;

Angle16 radiansToYaw(float radians)
{
    while (radians < 0.0f) {
        radians += kPi * 2.0f;
    }
    while (radians >= kPi * 2.0f) {
        radians -= kPi * 2.0f;
    }
    return static_cast<Angle16>(radians * 65536.0f / (kPi * 2.0f));
}

void addVertex(assets::Mesh& mesh, glm::vec3 position, glm::vec3 color)
{
    assets::MeshVertex vertex;
    vertex.position = position;
    vertex.normal = { 0.0f, 1.0f, 0.0f };
    vertex.color = { color.r, color.g, color.b, 1.0f };
    vertex.uv = { 0.0f, 0.0f };
    mesh.vertices.push_back(vertex);
}

void addTri(assets::Mesh& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 color)
{
    const glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
    const std::size_t first = mesh.vertices.size();
    addVertex(mesh, a, color);
    addVertex(mesh, b, color);
    addVertex(mesh, c, color);
    for (std::size_t i = first; i < mesh.vertices.size(); ++i) {
        mesh.vertices[i].normal = normal;
    }
}

void addBox(assets::Mesh& mesh, glm::vec3 center, glm::vec3 half, glm::vec3 color)
{
    const glm::vec3 p0 = center + glm::vec3(-half.x, -half.y, -half.z);
    const glm::vec3 p1 = center + glm::vec3(half.x, -half.y, -half.z);
    const glm::vec3 p2 = center + glm::vec3(half.x, half.y, -half.z);
    const glm::vec3 p3 = center + glm::vec3(-half.x, half.y, -half.z);
    const glm::vec3 p4 = center + glm::vec3(-half.x, -half.y, half.z);
    const glm::vec3 p5 = center + glm::vec3(half.x, -half.y, half.z);
    const glm::vec3 p6 = center + glm::vec3(half.x, half.y, half.z);
    const glm::vec3 p7 = center + glm::vec3(-half.x, half.y, half.z);

    addTri(mesh, p0, p1, p2, color); addTri(mesh, p0, p2, p3, color);
    addTri(mesh, p5, p4, p7, color); addTri(mesh, p5, p7, p6, color);
    addTri(mesh, p4, p0, p3, color); addTri(mesh, p4, p3, p7, color);
    addTri(mesh, p1, p5, p6, color); addTri(mesh, p1, p6, p2, color);
    addTri(mesh, p3, p2, p6, color); addTri(mesh, p3, p6, p7, color);
    addTri(mesh, p4, p5, p1, color); addTri(mesh, p4, p1, p0, color);
}

} // namespace

bool MarioKartBackend::initialize(const assets::RomImage& rom, const collision::CollisionWorld& collisionWorld)
{
    collisionWorld_ = &collisionWorld;
    active_ = true;
    body_ = MarioBody {};
    body_.position = { 0.0f, 80.0f, 0.0f };
    body_.grounded = false;
    yawRadians_ = 0.0f;
    speed_ = 0.0f;
    hopTimer_ = 0.0f;
    buildMesh();
    status_ = "Using Mario Kart 64 placeholder: ROM identified as " + rom.internalName
        + "; arcade kart movement active.";
    return true;
}

void MarioKartBackend::shutdown()
{
    active_ = false;
    collisionWorld_ = nullptr;
    mesh_.vertices.clear();
    status_ = "Mario Kart 64 backend not initialized";
}

void MarioKartBackend::setCameraLook(glm::vec3 look)
{
    look.y = 0.0f;
    if (glm::length(look) > 0.001f) {
        cameraLook_ = glm::normalize(look);
    }
}

void MarioKartBackend::tick(const MarioInput& input, float dt)
{
    if (!active_ || !collisionWorld_) {
        return;
    }

    const float throttle = std::clamp(input.stick.y, -1.0f, 1.0f);
    const float steer = std::clamp(input.stick.x, -1.0f, 1.0f);
    const float accel = throttle >= 0.0f ? 980.0f : 760.0f;
    speed_ += throttle * accel * dt;
    speed_ *= std::pow(input.crouchHeld ? 0.35f : 0.88f, dt * 6.0f);
    speed_ = std::clamp(speed_, -420.0f, 760.0f);

    const float steerStrength = std::clamp(std::abs(speed_) / 220.0f, 0.25f, 1.0f);
    yawRadians_ -= steer * steerStrength * 2.6f * dt * (speed_ >= 0.0f ? 1.0f : -1.0f);

    if (input.jumpPressed && body_.grounded && hopTimer_ <= 0.0f) {
        body_.velocity.y = 260.0f;
        body_.grounded = false;
        hopTimer_ = 0.28f;
    }
    hopTimer_ = std::max(0.0f, hopTimer_ - dt);

    const glm::vec3 forward(std::sin(yawRadians_), 0.0f, std::cos(yawRadians_));
    body_.velocity.x = forward.x * speed_;
    body_.velocity.z = forward.z * speed_;
    body_.velocity.y -= 980.0f * dt;
    body_.position += body_.velocity * dt;

    const auto collision = collisionWorld_->resolveCharacter(body_.position, 42.0f, 72.0f);
    body_.position = collision.correctedPosition;
    body_.grounded = collision.onFloor;
    body_.touchedWall = collision.hitWall;
    body_.touchedCeiling = collision.hitCeiling;
    body_.floorNormal = collision.floor ? collision.floor->normal : glm::vec3(0.0f, 1.0f, 0.0f);
    body_.wallNormal = collision.wall ? collision.wall->normal : glm::vec3(0.0f);

    if (collision.onFloor && body_.velocity.y < 0.0f) {
        body_.velocity.y = 0.0f;
    }
    if (collision.hitWall) {
        const glm::vec3 horizontalVelocity(body_.velocity.x, 0.0f, body_.velocity.z);
        const float intoWall = glm::dot(horizontalVelocity, body_.wallNormal);
        if (intoWall < 0.0f) {
            const glm::vec3 slide = horizontalVelocity - body_.wallNormal * intoWall;
            body_.velocity.x = slide.x;
            body_.velocity.z = slide.z;
            speed_ *= 0.35f;
        }
    }

    body_.faceYaw = radiansToYaw(yawRadians_);
    body_.intendedYaw = body_.faceYaw;
    body_.action = std::abs(speed_) > 40.0f ? Action::Running : Action::Idle;
}

void MarioKartBackend::syncBody(MarioBody& body) const
{
    if (active_) {
        body = body_;
    }
}

void MarioKartBackend::reloadSurfaces(const collision::CollisionWorld& collisionWorld)
{
    collisionWorld_ = &collisionWorld;
}

void MarioKartBackend::buildMesh()
{
    mesh_.name = "mk64_placeholder_kart";
    mesh_.worldSpace = false;
    mesh_.vertices.clear();
    addBox(mesh_, { 0.0f, 18.0f, 0.0f }, { 32.0f, 9.0f, 44.0f }, { 0.86f, 0.12f, 0.08f });
    addBox(mesh_, { 0.0f, 32.0f, -6.0f }, { 18.0f, 10.0f, 18.0f }, { 0.12f, 0.28f, 0.88f });
    addBox(mesh_, { -28.0f, 9.0f, -28.0f }, { 9.0f, 9.0f, 9.0f }, { 0.04f, 0.04f, 0.05f });
    addBox(mesh_, { 28.0f, 9.0f, -28.0f }, { 9.0f, 9.0f, 9.0f }, { 0.04f, 0.04f, 0.05f });
    addBox(mesh_, { -28.0f, 9.0f, 28.0f }, { 9.0f, 9.0f, 9.0f }, { 0.04f, 0.04f, 0.05f });
    addBox(mesh_, { 28.0f, 9.0f, 28.0f }, { 9.0f, 9.0f, 9.0f }, { 0.04f, 0.04f, 0.05f });
}

} // namespace sm64ps::mario
