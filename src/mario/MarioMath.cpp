#include "mario/MarioMath.h"

#include <algorithm>
#include <cmath>

namespace sm64ps::mario {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = kPi * 2.0f;

} // namespace

float angle16ToRadians(Angle16 angle)
{
    return static_cast<float>(angle) * kTwoPi / 65536.0f;
}

Angle16 radiansToAngle16(float radians)
{
    const float turns = radians / kTwoPi;
    const auto value = static_cast<int>(std::round(turns * 65536.0f));
    return static_cast<Angle16>(value & 0xffff);
}

std::int16_t signedAngleDelta(Angle16 from, Angle16 to)
{
    return static_cast<std::int16_t>(to - from);
}

Angle16 approachAngle(Angle16 current, Angle16 target, Angle16 step)
{
    const std::int16_t delta = signedAngleDelta(current, target);
    if (std::abs(delta) <= static_cast<int>(step)) {
        return target;
    }
    return static_cast<Angle16>(current + (delta > 0 ? step : -static_cast<int>(step)));
}

float approachFloat(float current, float target, float step)
{
    if (current < target) {
        return std::min(current + step, target);
    }
    return std::max(current - step, target);
}

glm::vec3 yawToForward(Angle16 yaw)
{
    const float radians = angle16ToRadians(yaw);
    return glm::vec3(std::sin(radians), 0.0f, std::cos(radians));
}

Angle16 forwardToYaw(const glm::vec3& forward)
{
    return radiansToAngle16(std::atan2(forward.x, forward.z));
}

float floorSlopeDegrees(const glm::vec3& normal)
{
    const float clampedY = std::clamp(normal.y, -1.0f, 1.0f);
    return std::acos(clampedY) * 180.0f / kPi;
}

float horizontalMagnitude(const glm::vec3& value)
{
    return std::sqrt(value.x * value.x + value.z * value.z);
}

} // namespace sm64ps::mario

