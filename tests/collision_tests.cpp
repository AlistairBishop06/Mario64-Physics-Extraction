#include "collision/CollisionWorld.h"

#include <cassert>
#include <iostream>

int main()
{
    sm64ps::collision::CollisionWorld world;
    world.buildTestArena();

    const auto floor = world.findFloor({ 0.0f, 4.0f, 0.0f }, 8.0f);
    assert(floor.has_value());
    assert(floor->normal.y > 0.99f);
    assert(floor->point.y == 0.0f);

    const auto penetratedFloor = world.findFloor({ 0.0f, -2.0f, 0.0f }, 8.0f);
    assert(penetratedFloor.has_value());
    assert(penetratedFloor->point.y == 0.0f);

    const auto resolved = world.resolveCharacter({ 0.0f, -2.0f, 0.0f }, 3.5f, 32.0f);
    assert(resolved.onFloor);
    assert(resolved.correctedPosition.y == 0.0f);

    std::cout << "collision tests passed\n";
    return 0;
}

