#include "assets/TestMap.h"
#include "collision/CollisionWorld.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

int main()
{
    using namespace sm64ps;

    const auto map = assets::TestMap::builtInMovementLab();
    assert(map.triangleCount() > 10);

    collision::CollisionWorld world;
    map.applyTo(world);
    assert(world.triangles().size() == map.triangleCount());
    assert(world.findFloor({ 0.0f, 2.0f, 0.0f }, 8.0f).has_value());
    assert(surfaces::sm64SurfaceTypeFor(surfaces::SurfaceType::Slippery) == 0x0014);
    assert(surfaces::sm64SurfaceTypeFor(surfaces::SurfaceType::VerySlippery) == 0x0013);
    assert(surfaces::sm64SurfaceTypeFor(surfaces::SurfaceType::NotSlippery) == 0x0015);
    assert(surfaces::sm64SurfaceTypeFor(surfaces::SurfaceType::Lava) == 0x0001);
    assert(surfaces::sm64SurfaceTypeFor(surfaces::SurfaceType::Quicksand) == 0x0026);
    assert(surfaces::sm64SurfaceTypeFor(surfaces::SurfaceType::WallKickable) == 0x0068);
    assert(surfaces::sm64TerrainFor(surfaces::SurfaceType::VerySlippery) == 0x0006);
    assert(surfaces::sm64TerrainFor(surfaces::SurfaceType::Quicksand) == 0x0003);

    const auto validPath = std::filesystem::temp_directory_path() / "sm64ps_valid_map.json";
    {
        std::ofstream file(validPath);
        file << R"({
            "name": "Unit Test Map",
            "meshes": [{
                "name": "Floor",
                "surface": "default",
                "color": [0.1, 0.2, 0.3],
                "triangles": [
                    [[0, 0, 0], [100, 0, 100], [100, 0, 0]]
                ]
            }]
        })";
    }

    std::string error;
    const auto parsed = assets::TestMap::loadJson(validPath, error);
    assert(parsed.has_value());
    assert(parsed->name() == "Unit Test Map");
    assert(parsed->triangleCount() == 1);
    std::filesystem::remove(validPath);

    const auto invalidPath = std::filesystem::temp_directory_path() / "sm64ps_invalid_map.json";
    {
        std::ofstream file(invalidPath);
        file << R"({
            "name": "Invalid",
            "meshes": [{
                "name": "Bad",
                "surface": "ice_but_not_supported",
                "triangles": [
                    [[0, 0, 0], [1, 0, 0], [0, 0, 1]]
                ]
            }]
        })";
    }

    error.clear();
    const auto invalid = assets::TestMap::loadJson(invalidPath, error);
    assert(!invalid.has_value());
    assert(error.find("invalid surface type") != std::string::npos);
    std::filesystem::remove(invalidPath);

    std::cout << "map tests passed\n";
    return 0;
}
