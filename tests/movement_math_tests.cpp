#include "mario/MarioMath.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    using namespace sm64ps::mario;

    assert(angle16ToRadians(kAngle0) == 0.0f);
    assert(std::abs(angle16ToRadians(kAngle180) - 3.14159265f) < 0.0001f);
    assert(radiansToAngle16(0.0f) == kAngle0);
    assert(radiansToAngle16(3.14159265f) == kAngle180);

    assert(signedAngleDelta(0xff00, 0x0100) == 0x0200);
    assert(signedAngleDelta(0x0100, 0xff00) == static_cast<std::int16_t>(-0x0200));

    assert(approachFloat(0.0f, 10.0f, 2.0f) == 2.0f);
    assert(approachFloat(9.0f, 10.0f, 2.0f) == 10.0f);
    assert(approachFloat(10.0f, 0.0f, 3.0f) == 7.0f);

    const glm::vec3 forward = yawToForward(kAngle0);
    assert(std::abs(forward.x) < 0.0001f);
    assert(std::abs(forward.z - 1.0f) < 0.0001f);

    std::cout << "movement math tests passed\n";
    return 0;
}
