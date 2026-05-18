#include "assets/TestMap.h"

#include "collision/CollisionWorld.h"

#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace sm64ps::assets {

namespace {

glm::vec3 parseVec3(const nlohmann::json& json, const std::string& field)
{
    if (!json.is_array() || json.size() != 3) {
        throw std::runtime_error(field + " must be an array of 3 numbers");
    }
    return {
        json.at(0).get<float>(),
        json.at(1).get<float>(),
        json.at(2).get<float>(),
    };
}

TestMapTriangle makeTriangle(glm::vec3 a, glm::vec3 b, glm::vec3 c,
    surfaces::SurfaceType surface, glm::vec3 color, std::string meshName)
{
    TestMapTriangle triangle;
    triangle.a = a;
    triangle.b = b;
    triangle.c = c;
    triangle.surface = surface;
    triangle.color = color;
    triangle.meshName = std::move(meshName);
    return triangle;
}

void addQuad(TestMapMesh& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d)
{
    mesh.triangles.push_back(makeTriangle(a, c, b, mesh.surface, mesh.color, mesh.name));
    mesh.triangles.push_back(makeTriangle(a, d, c, mesh.surface, mesh.color, mesh.name));
}

TestMapMesh makeMesh(std::string name, surfaces::SurfaceType surface, glm::vec3 color)
{
    TestMapMesh mesh;
    mesh.name = std::move(name);
    mesh.surface = surface;
    mesh.color = color;
    return mesh;
}

} // namespace

std::optional<surfaces::SurfaceType> surfaceTypeFromString(const std::string& name)
{
    if (name == "default") return surfaces::SurfaceType::Default;
    if (name == "slippery") return surfaces::SurfaceType::Slippery;
    if (name == "very_slippery") return surfaces::SurfaceType::VerySlippery;
    if (name == "not_slippery") return surfaces::SurfaceType::NotSlippery;
    if (name == "wall_kickable") return surfaces::SurfaceType::WallKickable;
    if (name == "ledge_grab") return surfaces::SurfaceType::LedgeGrab;
    if (name == "lava") return surfaces::SurfaceType::Lava;
    if (name == "quicksand") return surfaces::SurfaceType::Quicksand;
    return std::nullopt;
}

std::string surfaceTypeToString(surfaces::SurfaceType type)
{
    switch (type) {
    case surfaces::SurfaceType::Default: return "default";
    case surfaces::SurfaceType::Slippery: return "slippery";
    case surfaces::SurfaceType::VerySlippery: return "very_slippery";
    case surfaces::SurfaceType::NotSlippery: return "not_slippery";
    case surfaces::SurfaceType::WallKickable: return "wall_kickable";
    case surfaces::SurfaceType::LedgeGrab: return "ledge_grab";
    case surfaces::SurfaceType::Lava: return "lava";
    case surfaces::SurfaceType::Quicksand: return "quicksand";
    }
    return "unknown";
}

glm::vec3 defaultSurfaceColor(surfaces::SurfaceType type)
{
    switch (type) {
    case surfaces::SurfaceType::Default: return { 0.24f, 0.55f, 0.31f };
    case surfaces::SurfaceType::Slippery: return { 0.36f, 0.56f, 0.74f };
    case surfaces::SurfaceType::VerySlippery: return { 0.58f, 0.74f, 0.95f };
    case surfaces::SurfaceType::NotSlippery: return { 0.36f, 0.62f, 0.32f };
    case surfaces::SurfaceType::WallKickable: return { 0.64f, 0.66f, 0.72f };
    case surfaces::SurfaceType::LedgeGrab: return { 0.82f, 0.68f, 0.28f };
    case surfaces::SurfaceType::Lava: return { 0.95f, 0.18f, 0.08f };
    case surfaces::SurfaceType::Quicksand: return { 0.70f, 0.58f, 0.34f };
    }
    return { 0.8f, 0.8f, 0.8f };
}

TestMap TestMap::builtInMovementLab()
{
    TestMap map;
    map.name_ = "Built-in Movement Lab";

    auto floor = makeMesh("Large Floor", surfaces::SurfaceType::Default, defaultSurfaceColor(surfaces::SurfaceType::Default));
    addQuad(floor, { -3000.0f, 0.0f, -3000.0f }, { 3000.0f, 0.0f, -3000.0f }, { 3000.0f, 0.0f, 3000.0f },
        { -3000.0f, 0.0f, 3000.0f });
    map.meshes_.push_back(floor);

    auto slipperyRamp = makeMesh("Slippery 18 Degree Ramp", surfaces::SurfaceType::Slippery, defaultSurfaceColor(surfaces::SurfaceType::Slippery));
    addQuad(slipperyRamp, { 450.0f, 0.0f, -950.0f }, { 1450.0f, 0.0f, -950.0f }, { 1450.0f, 325.0f, 450.0f },
        { 450.0f, 325.0f, 450.0f });
    map.meshes_.push_back(slipperyRamp);

    auto slide = makeMesh("Very Slippery Slide", surfaces::SurfaceType::VerySlippery, defaultSurfaceColor(surfaces::SurfaceType::VerySlippery));
    addQuad(slide, { 950.0f, 0.0f, 1050.0f }, { 1600.0f, 0.0f, 1050.0f }, { 1600.0f, 650.0f, 1800.0f },
        { 950.0f, 650.0f, 1800.0f });
    map.meshes_.push_back(slide);

    auto wallKick = makeMesh("Wall Kick Corridor", surfaces::SurfaceType::WallKickable, defaultSurfaceColor(surfaces::SurfaceType::WallKickable));
    addQuad(wallKick, { -1450.0f, 0.0f, -900.0f }, { -1450.0f, 850.0f, -900.0f }, { -1450.0f, 850.0f, 900.0f },
        { -1450.0f, 0.0f, 900.0f });
    addQuad(wallKick, { -950.0f, 0.0f, 900.0f }, { -950.0f, 850.0f, 900.0f }, { -950.0f, 850.0f, -900.0f },
        { -950.0f, 0.0f, -900.0f });
    map.meshes_.push_back(wallKick);

    auto ledges = makeMesh("Ledge Platforms", surfaces::SurfaceType::LedgeGrab, defaultSurfaceColor(surfaces::SurfaceType::LedgeGrab));
    addQuad(ledges, { -550.0f, 260.0f, -1450.0f }, { 150.0f, 260.0f, -1450.0f }, { 150.0f, 260.0f, -850.0f },
        { -550.0f, 260.0f, -850.0f });
    addQuad(ledges, { 350.0f, 520.0f, -1450.0f }, { 950.0f, 520.0f, -1450.0f }, { 950.0f, 520.0f, -900.0f },
        { 350.0f, 520.0f, -900.0f });
    map.meshes_.push_back(ledges);

    auto runway = makeMesh("Long Jump Runway", surfaces::SurfaceType::NotSlippery, defaultSurfaceColor(surfaces::SurfaceType::NotSlippery));
    addQuad(runway, { -300.0f, 8.0f, 1300.0f }, { 300.0f, 8.0f, 1300.0f }, { 300.0f, 8.0f, 2800.0f },
        { -300.0f, 8.0f, 2800.0f });
    for (int i = 0; i < 5; ++i) {
        const float z = 1500.0f + static_cast<float>(i) * 250.0f;
        addQuad(runway, { -300.0f, 10.0f, z }, { 300.0f, 10.0f, z }, { 300.0f, 10.0f, z + 18.0f },
            { -300.0f, 10.0f, z + 18.0f });
    }
    map.meshes_.push_back(runway);

    auto stairs = makeMesh("Stairs", surfaces::SurfaceType::Default, { 0.40f, 0.44f, 0.48f });
    for (int i = 0; i < 7; ++i) {
        const float z0 = -2400.0f + static_cast<float>(i) * 135.0f;
        const float z1 = z0 + 135.0f;
        const float y = static_cast<float>(i + 1) * 55.0f;
        addQuad(stairs, { 1200.0f, y, z0 }, { 1800.0f, y, z0 }, { 1800.0f, y, z1 }, { 1200.0f, y, z1 });
    }
    map.meshes_.push_back(stairs);

    auto curved = makeMesh("Segmented Curved Slope", surfaces::SurfaceType::Slippery, { 0.30f, 0.50f, 0.62f });
    for (int i = 0; i < 8; ++i) {
        const float x0 = -2600.0f + static_cast<float>(i) * 170.0f;
        const float x1 = x0 + 170.0f;
        const float y0 = std::sin(static_cast<float>(i) * 0.32f) * 120.0f + 180.0f;
        const float y1 = std::sin(static_cast<float>(i + 1) * 0.32f) * 120.0f + 180.0f;
        addQuad(curved, { x0, y0, 1400.0f }, { x1, y1, 1400.0f }, { x1, y1, 2300.0f }, { x0, y0, 2300.0f });
    }
    map.meshes_.push_back(curved);

    auto hazards = makeMesh("Hazards", surfaces::SurfaceType::Lava, defaultSurfaceColor(surfaces::SurfaceType::Lava));
    addQuad(hazards, { -2600.0f, 4.0f, -2600.0f }, { -1900.0f, 4.0f, -2600.0f }, { -1900.0f, 4.0f, -1900.0f },
        { -2600.0f, 4.0f, -1900.0f });
    map.meshes_.push_back(hazards);

    auto quicksand = makeMesh("Quicksand Pit", surfaces::SurfaceType::Quicksand, defaultSurfaceColor(surfaces::SurfaceType::Quicksand));
    addQuad(quicksand, { 1900.0f, 3.0f, -250.0f }, { 2600.0f, 3.0f, -250.0f }, { 2600.0f, 3.0f, 450.0f },
        { 1900.0f, 3.0f, 450.0f });
    map.meshes_.push_back(quicksand);

    return map;
}

std::optional<TestMap> TestMap::loadJson(const std::filesystem::path& path, std::string& error)
{
    std::ifstream file(path);
    if (!file) {
        error = "unable to open map: " + path.string();
        return std::nullopt;
    }

    try {
        nlohmann::json json;
        file >> json;

        TestMap map;
        map.sourcePath_ = path;
        map.name_ = json.value("name", path.stem().string());

        if (!json.contains("meshes") || !json.at("meshes").is_array()) {
            throw std::runtime_error("map must contain a meshes array");
        }

        for (const auto& meshJson : json.at("meshes")) {
            TestMapMesh mesh;
            mesh.name = meshJson.value("name", "Unnamed Mesh");
            const std::string surfaceName = meshJson.value("surface", "default");
            const auto surface = surfaceTypeFromString(surfaceName);
            if (!surface) {
                throw std::runtime_error("invalid surface type '" + surfaceName + "' in mesh '" + mesh.name + "'");
            }
            mesh.surface = *surface;
            mesh.color = meshJson.contains("color") ? parseVec3(meshJson.at("color"), "mesh color") : defaultSurfaceColor(mesh.surface);

            if (!meshJson.contains("triangles") || !meshJson.at("triangles").is_array()) {
                throw std::runtime_error("mesh '" + mesh.name + "' must contain a triangles array");
            }

            for (const auto& triangleJson : meshJson.at("triangles")) {
                if (!triangleJson.is_array() || triangleJson.size() != 3) {
                    throw std::runtime_error("triangle in mesh '" + mesh.name + "' must contain exactly 3 vertices");
                }
                mesh.triangles.push_back(makeTriangle(
                    parseVec3(triangleJson.at(0), "triangle vertex"),
                    parseVec3(triangleJson.at(1), "triangle vertex"),
                    parseVec3(triangleJson.at(2), "triangle vertex"),
                    mesh.surface,
                    mesh.color,
                    mesh.name));
            }
            map.meshes_.push_back(std::move(mesh));
        }

        if (map.triangleCount() == 0) {
            throw std::runtime_error("map contains no triangles");
        }

        return map;
    } catch (const std::exception& exception) {
        error = exception.what();
        return std::nullopt;
    }
}

void TestMap::applyTo(collision::CollisionWorld& world) const
{
    world.clear();
    for (const auto& mesh : meshes_) {
        for (const auto& triangle : mesh.triangles) {
            world.addTriangle(collision::makeTriangle(triangle.a, triangle.b, triangle.c, triangle.surface));
        }
    }
}

std::size_t TestMap::triangleCount() const
{
    std::size_t count = 0;
    for (const auto& mesh : meshes_) {
        count += mesh.triangles.size();
    }
    return count;
}

} // namespace sm64ps::assets
