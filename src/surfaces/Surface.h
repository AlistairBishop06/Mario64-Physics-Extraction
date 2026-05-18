#pragma once

#include <cstdint>

namespace sm64ps::surfaces {

enum class SurfaceType : std::uint16_t {
    Default,
    Slippery,
    VerySlippery,
    NotSlippery,
    Lava,
    Quicksand,
    LedgeGrab,
    WallKickable
};

struct SurfaceProperties {
    SurfaceType type = SurfaceType::Default;
    float friction = 1.0f;
    float accelerationScale = 1.0f;
    float slideAcceleration = 0.0f;
    bool allowsWallKick = true;
    bool allowsLedgeGrab = false;
};

inline SurfaceProperties propertiesFor(SurfaceType type)
{
    switch (type) {
    case SurfaceType::Slippery: return { type, 0.55f, 0.75f, 1.2f, true, false };
    case SurfaceType::VerySlippery: return { type, 0.25f, 0.45f, 2.2f, true, false };
    case SurfaceType::NotSlippery: return { type, 1.25f, 1.1f, 0.0f, true, false };
    case SurfaceType::LedgeGrab: return { type, 1.0f, 1.0f, 0.0f, false, true };
    case SurfaceType::WallKickable: return { type, 1.0f, 1.0f, 0.0f, true, false };
    case SurfaceType::Lava: return { type, 0.75f, 0.7f, 0.0f, false, false };
    case SurfaceType::Quicksand: return { type, 0.3f, 0.25f, 0.0f, false, false };
    case SurfaceType::Default: break;
    }
    return { type, 1.0f, 1.0f, 0.0f, true, false };
}

inline std::int16_t sm64SurfaceTypeFor(SurfaceType type)
{
    switch (type) {
    case SurfaceType::Slippery: return 0x0014;
    case SurfaceType::VerySlippery: return 0x0013;
    case SurfaceType::NotSlippery: return 0x0015;
    case SurfaceType::Lava: return 0x0001;
    case SurfaceType::Quicksand: return 0x0026;
    case SurfaceType::LedgeGrab: return 0x0000;
    case SurfaceType::WallKickable: return 0x0068;
    case SurfaceType::Default: break;
    }
    return 0x0000;
}

inline std::uint16_t sm64TerrainFor(SurfaceType type)
{
    switch (type) {
    case SurfaceType::Slippery:
    case SurfaceType::VerySlippery:
        return 0x0006;
    case SurfaceType::NotSlippery:
    case SurfaceType::WallKickable:
    case SurfaceType::Lava:
        return 0x0001;
    case SurfaceType::Quicksand:
        return 0x0003;
    case SurfaceType::LedgeGrab:
    case SurfaceType::Default:
        break;
    }
    return 0x0000;
}

} // namespace sm64ps::surfaces
