#pragma once

#include "collision/CollisionTypes.h"

#include <vector>

namespace sm64ps::collision {

class CollisionWorld {
public:
    void clear();
    void addTriangle(const SurfaceTriangle& triangle);
    void addQuad(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
        surfaces::SurfaceType type = surfaces::SurfaceType::Default);
    void buildTestArena();

    std::optional<SurfaceContact> findFloor(const glm::vec3& position, float snapDistance) const;
    std::optional<SurfaceContact> findCeiling(const glm::vec3& position, float height) const;
    std::optional<SurfaceContact> findWall(const glm::vec3& position, float radius) const;
    CollisionResult resolveCharacter(const glm::vec3& position, float radius, float height) const;

    const std::vector<SurfaceTriangle>& triangles() const { return triangles_; }

private:
    std::vector<SurfaceTriangle> triangles_;
};

SurfaceTriangle makeTriangle(glm::vec3 a, glm::vec3 b, glm::vec3 c,
    surfaces::SurfaceType type = surfaces::SurfaceType::Default);

} // namespace sm64ps::collision

