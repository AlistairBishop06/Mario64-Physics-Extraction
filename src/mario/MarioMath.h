#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace sm64ps::mario {

using Angle16 = std::uint16_t;

constexpr Angle16 kAngle0 = 0x0000;
constexpr Angle16 kAngle90 = 0x4000;
constexpr Angle16 kAngle180 = 0x8000;
constexpr Angle16 kAngle270 = 0xC000;

float angle16ToRadians(Angle16 angle);
Angle16 radiansToAngle16(float radians);
std::int16_t signedAngleDelta(Angle16 from, Angle16 to);
Angle16 approachAngle(Angle16 current, Angle16 target, Angle16 step);
float approachFloat(float current, float target, float step);
glm::vec3 yawToForward(Angle16 yaw);
Angle16 forwardToYaw(const glm::vec3& forward);
float floorSlopeDegrees(const glm::vec3& normal);
float horizontalMagnitude(const glm::vec3& value);

} // namespace sm64ps::mario

