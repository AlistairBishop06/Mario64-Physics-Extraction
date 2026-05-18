#pragma once

#include <cstdint>

namespace sm64ps::physics {

class FixedTimestep {
public:
    explicit FixedTimestep(double fixedDelta = 1.0 / 30.0, std::uint32_t maxSteps = 5);

    template <typename StepFunc>
    std::uint32_t advance(double elapsedSeconds, StepFunc&& step)
    {
        accumulator_ += elapsedSeconds;
        std::uint32_t steps = 0;
        while (accumulator_ >= fixedDelta_ && steps < maxSteps_) {
            step(static_cast<float>(fixedDelta_));
            accumulator_ -= fixedDelta_;
            ++steps;
        }
        if (steps == maxSteps_ && accumulator_ >= fixedDelta_) {
            accumulator_ = 0.0;
        }
        return steps;
    }

    double alpha() const;
    double fixedDelta() const { return fixedDelta_; }

private:
    double fixedDelta_;
    double accumulator_ = 0.0;
    std::uint32_t maxSteps_;
};

} // namespace sm64ps::physics

