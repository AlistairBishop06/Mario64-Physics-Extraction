#pragma once

#include "mario/MarioMath.h"

#include <cstdint>
#include <string_view>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace sm64ps::mario {

enum class Action {
    Idle,
    Walking,
    Running,
    Crouching,
    Jump,
    DoubleJump,
    TripleJump,
    LongJump,
    Dive,
    Freefall,
    WallKick,
    Sliding,
    LedgeGrab
};

struct MarioInput {
    glm::vec2 stick { 0.0f, 0.0f };
    bool jumpPressed = false;
    bool crouchHeld = false;
    bool attackPressed = false;
};

struct MarioBody {
    glm::vec3 position { 0.0f, 20.0f, 0.0f };
    glm::vec3 velocity { 0.0f, 0.0f, 0.0f };
    glm::vec3 floorNormal { 0.0f, 1.0f, 0.0f };
    glm::vec3 wallNormal { 0.0f, 0.0f, 0.0f };
    Angle16 faceYaw = kAngle0;
    Angle16 intendedYaw = kAngle0;
    Action action = Action::Idle;
    bool grounded = false;
    bool touchedWall = false;
    bool touchedCeiling = false;
    std::uint32_t actionTimer = 0;
    std::uint8_t jumpCount = 0;
};

std::string_view actionName(Action action);

} // namespace sm64ps::mario

