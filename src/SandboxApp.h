#pragma once

#include "assets/RuntimeAssets.h"
#include "debug/DebugUI.h"
#include "debug/TweakVars.h"
#include "mario/LibSm64Backend.h"
#include "physics/FixedTimestep.h"
#include "physics/PhysicsWorld.h"
#include "rendering/DebugRenderer.h"
#include "rendering/Renderer.h"
#include "replay/Replay.h"

#include <SDL.h>

namespace sm64ps {

class SandboxApp {
public:
    bool initialize();
    int run();
    void shutdown();

private:
    mario::MarioInput pollInput(bool& quit);
    void registerConsoleCommands();
    void buildDebugDraw();

    SDL_Window* window_ = nullptr;
    rendering::Renderer renderer_;
    rendering::DebugRenderer debugRenderer_;
    physics::PhysicsWorld world_;
    physics::FixedTimestep timestep_;
    debug::TweakVars tweaks_ = debug::makeDefaultMovementTweaks();
    debug::DebugUI debugUi_;
    debug::DebugUiState debugState_;
    replay::ReplayTrack replay_;
    replay::ReplayTrack ghost_;
    assets::RuntimeAssets runtimeAssets_;
    mario::LibSm64Backend libSm64_;
};

} // namespace sm64ps
