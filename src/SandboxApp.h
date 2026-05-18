#pragma once

#include "assets/RuntimeAssets.h"
#include "assets/RomManager.h"
#include "assets/TestMap.h"
#include "debug/DebugUI.h"
#include "debug/TweakVars.h"
#include "mario/LibSm64Backend.h"
#include "mario/PlayerBackend.h"
#include "physics/FixedTimestep.h"
#include "physics/PhysicsWorld.h"
#include "rendering/DebugRenderer.h"
#include "rendering/Renderer.h"
#include "replay/Replay.h"

#include <filesystem>
#include <memory>
#include <vector>

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
    void updateRomRuntime();
    void spawnBackend(const assets::RomImage& rom);
    void despawnBackend(bool poof);
    void spawnPoof(glm::vec3 position);
    void updatePoof(float dt);
    void drawRomOverlay();
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
    assets::RomManager romManager_;
    std::unique_ptr<mario::PlayerBackend> playerBackend_;
    std::string activeRomSha1_;
    struct SmokeParticle {
        glm::vec3 position { 0.0f };
        glm::vec3 velocity { 0.0f };
        float age = 0.0f;
        float lifetime = 1.0f;
        float size = 10.0f;
    };
    std::vector<SmokeParticle> smoke_;
    float cameraOrbitInput_ = 0.0f;
    float cameraZoomInput_ = 0.0f;
    double audioAccumulator_ = 0.0;
};

} // namespace sm64ps
