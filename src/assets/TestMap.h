#pragma once

#include "collision/CollisionTypes.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace sm64ps::collision {
class CollisionWorld;
}

namespace sm64ps::assets {

struct TestMapTriangle {
    glm::vec3 a { 0.0f };
    glm::vec3 b { 0.0f };
    glm::vec3 c { 0.0f };
    surfaces::SurfaceType surface = surfaces::SurfaceType::Default;
    glm::vec3 color { 0.25f, 0.55f, 0.30f };
    std::string meshName;
};

struct TestMapMesh {
    std::string name;
    surfaces::SurfaceType surface = surfaces::SurfaceType::Default;
    glm::vec3 color { 0.25f, 0.55f, 0.30f };
    std::vector<TestMapTriangle> triangles;
};

class TestMap {
public:
    static TestMap builtInMovementLab();
    static std::optional<TestMap> loadJson(const std::filesystem::path& path, std::string& error);

    void applyTo(collision::CollisionWorld& world) const;

    const std::string& name() const { return name_; }
    const std::filesystem::path& sourcePath() const { return sourcePath_; }
    const std::vector<TestMapMesh>& meshes() const { return meshes_; }
    std::size_t triangleCount() const;

private:
    std::string name_ = "Untitled Map";
    std::filesystem::path sourcePath_;
    std::vector<TestMapMesh> meshes_;
};

std::optional<surfaces::SurfaceType> surfaceTypeFromString(const std::string& name);
std::string surfaceTypeToString(surfaces::SurfaceType type);
glm::vec3 defaultSurfaceColor(surfaces::SurfaceType type);

} // namespace sm64ps::assets

