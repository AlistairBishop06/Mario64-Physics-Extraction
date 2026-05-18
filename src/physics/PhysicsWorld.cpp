#include "physics/PhysicsWorld.h"

namespace sm64ps::physics {

PhysicsWorld::PhysicsWorld()
{
    reset();
}

void PhysicsWorld::reset()
{
    collisionWorld_.buildTestArena();
    marioBody_ = mario::MarioBody {};
    frame_ = 0;
}

void PhysicsWorld::step(const mario::MarioInput& input, float dt)
{
    marioController_.update(marioBody_, input, collisionWorld_, dt);
    advanceFrame();
}

void PhysicsWorld::advanceFrame()
{
    ++frame_;
}

} // namespace sm64ps::physics
