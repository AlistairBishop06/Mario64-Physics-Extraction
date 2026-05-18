#include "mario/ActionStateMachine.h"

namespace sm64ps::mario {

std::string_view actionName(Action action)
{
    switch (action) {
    case Action::Idle: return "Idle";
    case Action::Walking: return "Walking";
    case Action::Running: return "Running";
    case Action::Crouching: return "Crouching";
    case Action::Jump: return "Jump";
    case Action::DoubleJump: return "DoubleJump";
    case Action::TripleJump: return "TripleJump";
    case Action::LongJump: return "LongJump";
    case Action::Dive: return "Dive";
    case Action::Freefall: return "Freefall";
    case Action::WallKick: return "WallKick";
    case Action::Sliding: return "Sliding";
    case Action::LedgeGrab: return "LedgeGrab";
    }
    return "Unknown";
}

Action ActionStateMachine::update(const MarioBody& body, const MarioInput& input, const TransitionContext& context) const
{
    if (!body.grounded) {
        if (context.ledgeGrabbable) {
            return Action::LedgeGrab;
        }
        if (input.jumpPressed && body.touchedWall && context.wallKickable) {
            return Action::WallKick;
        }
        if (input.attackPressed) {
            return Action::Dive;
        }
        if (body.action == Action::Jump || body.action == Action::DoubleJump || body.action == Action::TripleJump
            || body.action == Action::LongJump || body.action == Action::WallKick || body.action == Action::Dive) {
            return body.action;
        }
        return Action::Freefall;
    }

    if (context.floorSlopeDegrees > 38.0f && context.stickMagnitude < 0.25f) {
        return Action::Sliding;
    }

    if (input.crouchHeld && input.jumpPressed && context.stickMagnitude > 0.35f) {
        return Action::LongJump;
    }
    if (input.crouchHeld) {
        return Action::Crouching;
    }
    if (input.jumpPressed) {
        if (body.jumpCount >= 2) {
            return Action::TripleJump;
        }
        if (body.jumpCount == 1) {
            return Action::DoubleJump;
        }
        return Action::Jump;
    }
    if (context.stickMagnitude > 0.75f) {
        return Action::Running;
    }
    if (context.stickMagnitude > 0.08f) {
        return Action::Walking;
    }
    return Action::Idle;
}

} // namespace sm64ps::mario

