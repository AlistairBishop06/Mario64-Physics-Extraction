#pragma once

#include "debug/Console.h"
#include "debug/TweakVars.h"
#include "physics/PhysicsWorld.h"
#include "replay/Replay.h"

#include <array>
#include <string>

namespace sm64ps::debug {

struct DebugUiState {
    bool drawVelocity = true;
    bool drawNormals = true;
    bool drawCollision = true;
    bool paused = false;
    bool frameStepRequested = false;
    bool recording = true;
    int replayFrame = 0;
    std::string backendStatus;
};

class DebugUI {
public:
    DebugUI();

    void draw(physics::PhysicsWorld& world, TweakVars& tweaks, replay::ReplayTrack& replay,
        const replay::ReplayTrack& ghost, DebugUiState& state);

    Console& console() { return console_; }

private:
    void drawConsole();

    Console console_;
    std::array<char, 256> commandBuffer_ {};
    std::string consoleOutput_;
};

} // namespace sm64ps::debug
