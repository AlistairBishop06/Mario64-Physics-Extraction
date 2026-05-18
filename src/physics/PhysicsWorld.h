#pragma once

#include "collision/CollisionWorld.h"
#include "mario/MarioController.h"

#include <cstdint>

namespace sm64ps::physics {

class PhysicsWorld {
public:
    PhysicsWorld();

    void reset();
    void step(const mario::MarioInput& input, float dt);

    collision::CollisionWorld& collisionWorld() { return collisionWorld_; }
    const collision::CollisionWorld& collisionWorld() const { return collisionWorld_; }

    mario::MarioBody& marioBody() { return marioBody_; }
    const mario::MarioBody& marioBody() const { return marioBody_; }

    mario::MarioController& marioController() { return marioController_; }
    const mario::MarioController& marioController() const { return marioController_; }

    std::uint64_t frame() const { return frame_; }

private:
    collision::CollisionWorld collisionWorld_;
    mario::MarioController marioController_;
    mario::MarioBody marioBody_;
    std::uint64_t frame_ = 0;
};

} // namespace sm64ps::physics

