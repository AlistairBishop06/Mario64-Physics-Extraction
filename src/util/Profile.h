#pragma once

#include "util/Log.h"

#include <chrono>
#include <string>

namespace sm64ps::util {

class ScopedProfile {
public:
    explicit ScopedProfile(std::string name)
        : name_(std::move(name))
        , start_(std::chrono::high_resolution_clock::now())
    {
    }

    ~ScopedProfile()
    {
        const auto end = std::chrono::high_resolution_clock::now();
        const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        logDebug("[profile] ", name_, " ", microseconds, "us");
    }

private:
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_;
};

} // namespace sm64ps::util

