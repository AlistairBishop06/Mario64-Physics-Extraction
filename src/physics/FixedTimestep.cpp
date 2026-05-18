#include "physics/FixedTimestep.h"

namespace sm64ps::physics {

FixedTimestep::FixedTimestep(double fixedDelta, std::uint32_t maxSteps)
    : fixedDelta_(fixedDelta)
    , maxSteps_(maxSteps)
{
}

double FixedTimestep::alpha() const
{
    return fixedDelta_ > 0.0 ? accumulator_ / fixedDelta_ : 0.0;
}

} // namespace sm64ps::physics

