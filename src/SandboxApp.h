#pragma once

#include "assets/RuntimeAssets.h"
#include "assets/TestMap.h"
#include "debug/DebugUI.h"
#include "debug/TweakVars.h"
#include "mario/LibSm64Backend.h"
#include "physics/FixedTimestep.h"
#include "physics/PhysicsWorld.h"
#include "rendering/DebugRenderer.h"
#include "rendering/Renderer.h"
#include "replay/Replay.h"

#include <filesystem>

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
    bool initializeAudio();
    void pumpAudio();
    void shutdownAudio();
    bool loadMap(const std::filesystem::path& path);
    void loadDefaultMap();
    void applyActiveMap();
    void refreshMapDebugState();

    SDL_Window* window_ = nullptr;
    SDL_AudioDeviceID audioDevice_ = 0;
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
    assets::TestMap currentMap_;
    std::filesystem::path currentMapPath_;
    mario::LibSm64Backend libSm64_;
    float cameraOrbitInput_ = 0.0f;
    float cameraZoomInput_ = 0.0f;
    double audioAccumulator_ = 0.0;
};

} // namespace sm64ps
