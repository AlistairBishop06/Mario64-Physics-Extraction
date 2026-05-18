#include "mario/MarioController.h"

#include "mario/MarioMath.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace sm64ps::mario {

namespace {

float stickMagnitude(const MarioInput& input)
{
    return std::clamp(glm::length(input.stick), 0.0f, 1.0f);
}

} // namespace

MarioController::MarioController(MovementTuning tuning)
    : tuning_(tuning)
{
}

void MarioController::update(MarioBody& body, const MarioInput& input, const collision::CollisionWorld& collisionWorld, float dt)
{
    const auto preContact = collisionWorld.resolveCharacter(body.position, tuning_.characterRadius, tuning_.characterHeight);
    body.grounded = preContact.onFloor;
    body.touchedWall = preContact.hitWall;
    body.touchedCeiling = preContact.hitCeiling;
    body.floorNormal = preContact.floor ? preContact.floor->normal : glm::vec3(0.0f, 1.0f, 0.0f);
    body.wallNormal = preContact.wall ? preContact.wall->normal : glm::vec3(0.0f);

    const surfaces::SurfaceProperties surface = preContact.floor && preContact.floor->surface
        ? surfaces::propertiesFor(preContact.floor->surface->type)
        : surfaces::propertiesFor(surfaces::SurfaceType::Default);

    TransitionContext context;
    context.stickMagnitude = stickMagnitude(input);
    context.floorSlopeDegrees = floorSlopeDegrees(body.floorNormal);
    context.wallKickable = preContact.wall && preContact.wall->surface
        ? surfaces::propertiesFor(preContact.wall->surface->type).allowsWallKick
        : false;
    context.ledgeGrabbable = preContact.wall && preContact.wall->surface
        ? surfaces::propertiesFor(preContact.wall->surface->type).allowsLedgeGrab
        : false;

    updateYaw(body, input);

    const Action previousAction = body.action;
    const Action nextAction = stateMachine_.update(body, input, context);
    if (nextAction != previousAction) {
        body.actionTimer = 0;
        body.action = nextAction;
        applyJumpImpulse(body, nextAction);
    } else {
        ++body.actionTimer;
    }

    if (body.grounded) {
        applyGroundMovement(body, input, surface, dt);
    } else {
        applyAirMovement(body, input, dt);
    }

    const glm::vec3 previousPosition = body.position;
    body.velocity.y = std::max(body.velocity.y + tuning_.gravity * dt * 30.0f, tuning_.terminalVelocity);
    body.position += body.velocity * dt;

    auto postContact = collisionWorld.resolveCharacter(body.position, tuning_.characterRadius, tuning_.characterHeight);
    if (!postContact.onFloor && body.velocity.y <= 0.0f) {
        const float fallDistance = std::max(previousPosition.y - body.position.y, 0.0f);
        if (auto floor = collisionWorld.findFloor(body.position, fallDistance + 8.0f)) {
            postContact.floor = floor;
            postContact.onFloor = true;
            postContact.correctedPosition.y = floor->point.y;
        }
    }
    body.position = postContact.correctedPosition;
    body.grounded = postContact.onFloor;
    body.touchedWall = postContact.hitWall;
    body.touchedCeiling = postContact.hitCeiling;
    body.floorNormal = postContact.floor ? postContact.floor->normal : body.floorNormal;
    body.wallNormal = postContact.wall ? postContact.wall->normal : glm::vec3(0.0f);

    if (body.grounded && body.velocity.y <= 0.0f) {
        body.velocity.y = 0.0f;
        if (body.action == Action::Freefall || body.action == Action::Dive || body.action == Action::WallKick) {
            body.action = horizontalMagnitude(body.velocity) > 1.0f ? Action::Walking : Action::Idle;
        }
    }

    if (body.touchedCeiling && body.velocity.y > 0.0f) {
        body.velocity.y = 0.0f;
    }
}

void MarioController::updateYaw(MarioBody& body, const MarioInput& input) const
{
    if (glm::length(input.stick) < 0.08f) {
        return;
    }

    body.intendedYaw = forwardToYaw(glm::vec3(input.stick.x, 0.0f, input.stick.y));
    const Angle16 turnRate = body.grounded ? 0x0800 : 0x0300;
    body.faceYaw = approachAngle(body.faceYaw, body.intendedYaw, turnRate);
}

void MarioController::applyGroundMovement(MarioBody& body, const MarioInput& input, const surfaces::SurfaceProperties& surface, float dt) const
{
    const float magnitude = stickMagnitude(input);
    const float maxSpeed = magnitude > 0.75f ? tuning_.runSpeed : tuning_.walkSpeed;
    const float targetSpeed = maxSpeed * magnitude;
    const float currentSpeed = horizontalMagnitude(body.velocity);
    const float accel = (targetSpeed > currentSpeed ? tuning_.groundAccel : tuning_.groundDecel) * surface.accelerationScale;
    const float newSpeed = approachFloat(currentSpeed, targetSpeed, accel * dt * 30.0f);
    const glm::vec3 forward = yawToForward(body.faceYaw);

    body.velocity.x = forward.x * newSpeed;
    body.velocity.z = forward.z * newSpeed;

    if (body.action == Action::Sliding || surface.slideAcceleration > 0.0f) {
        glm::vec3 downhill(body.floorNormal.x, 0.0f, body.floorNormal.z);
        if (glm::length(downhill) > 0.001f) {
            downhill = glm::normalize(downhill);
            body.velocity += downhill * surface.slideAcceleration * dt * 30.0f;
        }
    }
}

void MarioController::applyAirMovement(MarioBody& body, const MarioInput& input, float dt) const
{
    if (stickMagnitude(input) <= 0.08f) {
        return;
    }

    const glm::vec3 intended = yawToForward(body.intendedYaw);
    body.velocity.x += intended.x * tuning_.airAccel * dt * 30.0f;
    body.velocity.z += intended.z * tuning_.airAccel * dt * 30.0f;

    const float speed = horizontalMagnitude(body.velocity);
    const float cap = tuning_.runSpeed * 1.35f;
    if (speed > cap) {
        const float scale = cap / speed;
        body.velocity.x *= scale;
        body.velocity.z *= scale;
    }
}

void MarioController::applyJumpImpulse(MarioBody& body, Action newAction) const
{
    switch (newAction) {
    case Action::Jump:
        body.velocity.y = tuning_.jumpVelocity;
        body.grounded = false;
        body.jumpCount = 1;
        break;
    case Action::DoubleJump:
        body.velocity.y = tuning_.jumpVelocity * 1.12f;
        body.grounded = false;
        body.jumpCount = 2;
        break;
    case Action::TripleJump:
        body.velocity.y = tuning_.jumpVelocity * 1.28f;
        body.grounded = false;
        body.jumpCount = 3;
        break;
    case Action::LongJump: {
        const glm::vec3 forward = yawToForward(body.faceYaw);
        body.velocity.x = forward.x * tuning_.longJumpForwardSpeed;
        body.velocity.z = forward.z * tuning_.longJumpForwardSpeed;
        body.velocity.y = tuning_.jumpVelocity * 0.78f;
        body.grounded = false;
        break;
    }
    case Action::Dive: {
        const glm::vec3 forward = yawToForward(body.faceYaw);
        body.velocity.x = forward.x * tuning_.diveForwardSpeed;
        body.velocity.z = forward.z * tuning_.diveForwardSpeed;
        body.velocity.y = tuning_.jumpVelocity * 0.25f;
        break;
    }
    case Action::WallKick: {
        const glm::vec3 away = glm::length(body.wallNormal) > 0.001f ? body.wallNormal : -yawToForward(body.faceYaw);
        body.velocity.x = away.x * tuning_.wallKickSpeed;
        body.velocity.z = away.z * tuning_.wallKickSpeed;
        body.velocity.y = tuning_.jumpVelocity * 1.1f;
        body.faceYaw = forwardToYaw(away);
        break;
    }
    default:
        if (body.grounded) {
            body.jumpCount = 0;
        }
        break;
    }
}

} // namespace sm64ps::mario
