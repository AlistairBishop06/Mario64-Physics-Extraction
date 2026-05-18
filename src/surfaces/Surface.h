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

} // namespace sm64ps::surfaces

