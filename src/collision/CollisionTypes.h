#pragma once

#include "surfaces/Surface.h"

#include <cstddef>
#include <optional>

#include <glm/vec3.hpp>

namespace sm64ps::collision {

struct SurfaceTriangle {
    glm::vec3 a { 0.0f };
    glm::vec3 b { 0.0f };
    glm::vec3 c { 0.0f };
    glm::vec3 normal { 0.0f, 1.0f, 0.0f };
    surfaces::SurfaceType type = surfaces::SurfaceType::Default;
};

struct SurfaceContact {
    const SurfaceTriangle* surface = nullptr;
    glm::vec3 point { 0.0f };
    glm::vec3 normal { 0.0f, 1.0f, 0.0f };
    float distance = 0.0f;
};

struct CollisionResult {
    glm::vec3 correctedPosition { 0.0f };
    bool onFloor = false;
    bool hitWall = false;
    bool hitCeiling = false;
    std::optional<SurfaceContact> floor;
    std::optional<SurfaceContact> wall;
    std::optional<SurfaceContact> ceiling;
};

} // namespace sm64ps::collision

