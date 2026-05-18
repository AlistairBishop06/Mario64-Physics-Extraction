#include "collision/CollisionWorld.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

namespace sm64ps::collision {

namespace {

bool pointInTriangleXZ(const glm::vec3& point, const SurfaceTriangle& triangle)
{
    const glm::vec2 p(point.x, point.z);
    const glm::vec2 a(triangle.a.x, triangle.a.z);
    const glm::vec2 b(triangle.b.x, triangle.b.z);
    const glm::vec2 c(triangle.c.x, triangle.c.z);

    const auto sign = [](const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    };

    const float d1 = sign(p, a, b);
    const float d2 = sign(p, b, c);
    const float d3 = sign(p, c, a);

    const bool hasNegative = d1 < 0.0f || d2 < 0.0f || d3 < 0.0f;
    const bool hasPositive = d1 > 0.0f || d2 > 0.0f || d3 > 0.0f;
    return !(hasNegative && hasPositive);
}

float yOnPlane(const SurfaceTriangle& triangle, const glm::vec3& position)
{
    if (std::abs(triangle.normal.y) < 0.0001f) {
        return triangle.a.y;
    }

    return triangle.a.y
        - (triangle.normal.x * (position.x - triangle.a.x) + triangle.normal.z * (position.z - triangle.a.z))
            / triangle.normal.y;
}

float distancePointToSegmentXZ(const glm::vec3& point, const glm::vec3& a, const glm::vec3& b, glm::vec3& closest)
{
    const glm::vec2 p(point.x, point.z);
    const glm::vec2 va(a.x, a.z);
    const glm::vec2 vb(b.x, b.z);
    const glm::vec2 ab = vb - va;
    const float denom = glm::dot(ab, ab);
    const float t = denom > 0.0f ? std::clamp(glm::dot(p - va, ab) / denom, 0.0f, 1.0f) : 0.0f;
    const glm::vec2 hit = va + ab * t;
    closest = glm::vec3(hit.x, point.y, hit.y);
    return glm::length(p - hit);
}

} // namespace

SurfaceTriangle makeTriangle(glm::vec3 a, glm::vec3 b, glm::vec3 c, surfaces::SurfaceType type)
{
    SurfaceTriangle triangle;
    triangle.a = a;
    triangle.b = b;
    triangle.c = c;
    triangle.type = type;
    triangle.normal = glm::normalize(glm::cross(b - a, c - a));
    if (!std::isfinite(triangle.normal.x)) {
        triangle.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    return triangle;
}

void CollisionWorld::clear()
{
    triangles_.clear();
}

void CollisionWorld::addTriangle(const SurfaceTriangle& triangle)
{
    triangles_.push_back(triangle);
}

void CollisionWorld::addQuad(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
    surfaces::SurfaceType type)
{
    addTriangle(makeTriangle(a, c, b, type));
    addTriangle(makeTriangle(a, d, c, type));
}

void CollisionWorld::buildTestArena()
{
    clear();
    addQuad({ -120.0f, 0.0f, -120.0f }, { 120.0f, 0.0f, -120.0f }, { 120.0f, 0.0f, 120.0f },
        { -120.0f, 0.0f, 120.0f });
    addQuad({ 20.0f, 0.0f, -40.0f }, { 80.0f, 0.0f, -40.0f }, { 80.0f, 35.0f, 40.0f },
        { 20.0f, 35.0f, 40.0f }, surfaces::SurfaceType::Slippery);
    addQuad({ -80.0f, 0.0f, 70.0f }, { -80.0f, 60.0f, 70.0f }, { -80.0f, 60.0f, -70.0f },
        { -80.0f, 0.0f, -70.0f }, surfaces::SurfaceType::WallKickable);
}

std::optional<SurfaceContact> CollisionWorld::findFloor(const glm::vec3& position, float snapDistance) const
{
    std::optional<SurfaceContact> best;
    float bestDistance = std::numeric_limits<float>::max();

    for (const SurfaceTriangle& triangle : triangles_) {
        if (triangle.normal.y <= 0.01f || !pointInTriangleXZ(position, triangle)) {
            continue;
        }

        const float floorY = yOnPlane(triangle, position);
        const float distance = position.y - floorY;
        if (distance >= -0.05f && distance <= snapDistance && distance < bestDistance) {
            bestDistance = distance;
            best = SurfaceContact { &triangle, glm::vec3(position.x, floorY, position.z), triangle.normal, distance };
        }
    }

    return best;
}

std::optional<SurfaceContact> CollisionWorld::findCeiling(const glm::vec3& position, float height) const
{
    std::optional<SurfaceContact> best;
    float bestDistance = std::numeric_limits<float>::max();
    const glm::vec3 head = position + glm::vec3(0.0f, height, 0.0f);

    for (const SurfaceTriangle& triangle : triangles_) {
        if (triangle.normal.y >= -0.01f || !pointInTriangleXZ(head, triangle)) {
            continue;
        }

        const float ceilingY = yOnPlane(triangle, head);
        const float distance = ceilingY - head.y;
        if (distance >= -0.05f && distance < bestDistance) {
            bestDistance = distance;
            best = SurfaceContact { &triangle, glm::vec3(head.x, ceilingY, head.z), triangle.normal, distance };
        }
    }

    return best;
}

std::optional<SurfaceContact> CollisionWorld::findWall(const glm::vec3& position, float radius) const
{
    std::optional<SurfaceContact> best;
    float bestDistance = radius;

    for (const SurfaceTriangle& triangle : triangles_) {
        if (std::abs(triangle.normal.y) > 0.2f) {
            continue;
        }

        const glm::vec3 edges[] = { triangle.a, triangle.b, triangle.c, triangle.a };
        for (int i = 0; i < 3; ++i) {
            glm::vec3 closest {};
            const float distance = distancePointToSegmentXZ(position, edges[i], edges[i + 1], closest);
            const float minY = std::min(edges[i].y, edges[i + 1].y) - 1.0f;
            const float maxY = std::max(edges[i].y, edges[i + 1].y) + 80.0f;
            if (distance < bestDistance && position.y >= minY && position.y <= maxY) {
                bestDistance = distance;
                glm::vec3 normal = position - closest;
                normal.y = 0.0f;
                if (glm::length(normal) < 0.0001f) {
                    normal = triangle.normal;
                    normal.y = 0.0f;
                }
                normal = glm::normalize(normal);
                best = SurfaceContact { &triangle, closest, normal, radius - distance };
            }
        }
    }

    return best;
}

CollisionResult CollisionWorld::resolveCharacter(const glm::vec3& position, float radius, float height) const
{
    CollisionResult result;
    result.correctedPosition = position;

    if (auto floor = findFloor(result.correctedPosition, 8.0f)) {
        result.correctedPosition.y = floor->point.y;
        result.floor = floor;
        result.onFloor = true;
    }

    if (auto wall = findWall(result.correctedPosition, radius)) {
        result.correctedPosition += wall->normal * wall->distance;
        result.wall = wall;
        result.hitWall = true;
    }

    if (auto ceiling = findCeiling(result.correctedPosition, height)) {
        result.ceiling = ceiling;
        result.hitCeiling = true;
    }

    return result;
}

} // namespace sm64ps::collision
