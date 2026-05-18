#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace sm64ps::assets {

struct MeshVertex {
    glm::vec3 position { 0.0f };
    glm::vec3 normal { 0.0f, 1.0f, 0.0f };
    glm::vec4 color { 1.0f };
    glm::vec2 uv { 0.0f };
};

struct Mesh {
    std::string name;
    std::vector<MeshVertex> vertices;
    bool worldSpace = false;
    bool empty() const { return vertices.empty(); }
};

class RuntimeAssets {
public:
    bool loadFromDirectory(const std::filesystem::path& directory);

    const Mesh& marioMesh() const { return marioMesh_; }
    bool hasMarioMesh() const { return !marioMesh_.empty(); }
    const std::string& status() const { return status_; }

private:
    bool loadObj(const std::filesystem::path& path, Mesh& mesh);

    Mesh marioMesh_;
    std::string status_ = "No extracted runtime assets loaded.";
};

} // namespace sm64ps::assets
