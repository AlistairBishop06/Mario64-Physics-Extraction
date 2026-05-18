#pragma once

#include "collision/CollisionWorld.h"
#include "mario/ActionStateMachine.h"

namespace sm64ps::mario {

struct MovementTuning {
    float walkSpeed = 12.0f;
    float runSpeed = 32.0f;
    float groundAccel = 2.4f;
    float groundDecel = 1.8f;
    float airAccel = 1.1f;
    float jumpVelocity = 42.0f;
    float gravity = -4.0f;
    float terminalVelocity = -75.0f;
    float longJumpForwardSpeed = 48.0f;
    float diveForwardSpeed = 52.0f;
    float wallKickSpeed = 34.0f;
    float characterRadius = 3.5f;
    float characterHeight = 32.0f;
};

class MarioController {
public:
    explicit MarioController(MovementTuning tuning = {});

    void update(MarioBody& body, const MarioInput& input, const collision::CollisionWorld& collisionWorld, float dt);
    const MovementTuning& tuning() const { return tuning_; }
    MovementTuning& tuning() { return tuning_; }

private:
    void updateYaw(MarioBody& body, const MarioInput& input) const;
    void applyGroundMovement(MarioBody& body, const MarioInput& input, const surfaces::SurfaceProperties& surface, float dt) const;
    void applyAirMovement(MarioBody& body, const MarioInput& input, float dt) const;
    void applyJumpImpulse(MarioBody& body, Action newAction) const;

    MovementTuning tuning_;
    ActionStateMachine stateMachine_;
};

} // namespace sm64ps::mario

