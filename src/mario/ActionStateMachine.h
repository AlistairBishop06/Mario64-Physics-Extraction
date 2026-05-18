#pragma once

#include "mario/MarioState.h"

namespace sm64ps::mario {

struct TransitionContext {
    float stickMagnitude = 0.0f;
    float floorSlopeDegrees = 0.0f;
    bool wallKickable = false;
    bool ledgeGrabbable = false;
};

class ActionStateMachine {
public:
    Action update(const MarioBody& body, const MarioInput& input, const TransitionContext& context) const;
};

} // namespace sm64ps::mario

