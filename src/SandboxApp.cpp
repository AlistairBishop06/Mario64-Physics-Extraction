#include "SandboxApp.h"

#include "mario/MarioKartBackend.h"
#include "mario/MarioMath.h"
#include "util/Config.h"
#include "util/Log.h"

#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <glm/geometric.hpp>

namespace sm64ps {

namespace {

std::filesystem::path executableDirectory()
{
    char* basePath = SDL_GetBasePath();
    if (!basePath) {
        return std::filesystem::current_path();
    }

    std::filesystem::path path(basePath);
    SDL_free(basePath);
    return path;
}

std::vector<std::filesystem::path> searchRoots()
{
    const std::filesystem::path exeDir = executableDirectory();
    return {
        exeDir,
        exeDir / "..",
        exeDir / ".." / "..",
        std::filesystem::current_path(),
    };
}

std::optional<std::filesystem::path> findFirstExisting(const std::vector<std::filesystem::path>& candidates)
{
    for (const auto& candidate : candidates) {
        const auto normalized = std::filesystem::weakly_canonical(candidate);
        if (std::filesystem::exists(normalized)) {
            return normalized;
        }
    }
    return std::nullopt;
}

std::filesystem::path findConfigPath()
{
    std::vector<std::filesystem::path> candidates;
    for (const auto& root : searchRoots()) {
        candidates.push_back(root / "configs" / "default.json");
    }
    return findFirstExisting(candidates).value_or("configs/default.json");
}

std::filesystem::path findExtractedDirectory()
{
    std::vector<std::filesystem::path> candidates;
    for (const auto& root : searchRoots()) {
        candidates.push_back(root / "extracted");
    }
    return findFirstExisting(candidates).value_or("extracted");
}

std::filesystem::path findRomDirectory()
{
    std::vector<std::filesystem::path> candidates;
    for (const auto& root : searchRoots()) {
        candidates.push_back(root / "roms");
    }
    return findFirstExisting(candidates).value_or("roms");
}

std::filesystem::path findDefaultMapPath()
{
    std::vector<std::filesystem::path> candidates;
    for (const auto& root : searchRoots()) {
        candidates.push_back(root / "maps" / "movement_lab.json");
    }
    return findFirstExisting(candidates).value_or("maps/movement_lab.json");
}

std::filesystem::path findLibSm64Dll()
{
    std::vector<std::filesystem::path> candidates;
    for (const auto& root : searchRoots()) {
        candidates.push_back(root / "sm64.dll");
        candidates.push_back(root / "third_party" / "libsm64" / "dist" / "sm64.dll");
    }
    return findFirstExisting(candidates).value_or(mario::LibSm64Backend::findDefaultLibrary());
}

} // namespace

bool SandboxApp::initialize()
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
        util::logError("SDL_Init failed: ", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window_ = SDL_CreateWindow("SM64 Physics Sandbox", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        util::logError("SDL_CreateWindow failed: ", SDL_GetError());
        return false;
    }

    if (!renderer_.initialize(window_)) {
        util::logError("OpenGL context failed: ", SDL_GetError());
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window_, SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL3_Init("#version 120");

    util::Config config;
    if (config.load(findConfigPath())) {
        tweaks_.setNumber("walk_speed", config.value("/movement/walk_speed", 12.0));
        tweaks_.setNumber("run_speed", config.value("/movement/run_speed", 32.0));
        tweaks_.setNumber("ground_accel", config.value("/movement/ground_accel", 2.4));
        tweaks_.setNumber("ground_decel", config.value("/movement/ground_decel", 1.8));
        tweaks_.setNumber("air_accel", config.value("/movement/air_accel", 1.1));
        tweaks_.setNumber("jump_velocity", config.value("/movement/jump_velocity", 42.0));
        tweaks_.setNumber("gravity", config.value("/movement/gravity", -4.0));
        tweaks_.setNumber("terminal_velocity", config.value("/movement/terminal_velocity", -75.0));
        tweaks_.setNumber("long_jump_forward_speed", config.value("/movement/long_jump_forward_speed", 48.0));
        tweaks_.setNumber("dive_forward_speed", config.value("/movement/dive_forward_speed", 52.0));
    }
    tweaks_.applyTo(world_.marioController().tuning());
    runtimeAssets_.loadFromDirectory(findExtractedDirectory());
    loadDefaultMap();
    romManager_ = assets::RomManager(findRomDirectory());
    updateRomRuntime();
    refreshMapDebugState();
    registerConsoleCommands();
    return true;
}

int SandboxApp::run()
{
    bool quit = false;
    auto previous = std::chrono::high_resolution_clock::now();

    while (!quit) {
        const auto now = std::chrono::high_resolution_clock::now();
        const double elapsed = std::chrono::duration<double>(now - previous).count();
        previous = now;

        mario::MarioInput input = pollInput(quit);
        updateRomRuntime();
        renderer_.updateLakituCamera(world_.marioBody(), cameraOrbitInput_, cameraZoomInput_);

        const bool shouldStep = !debugState_.paused || debugState_.frameStepRequested;
        if (shouldStep) {
            timestep_.advance(elapsed, [&](float dt) {
                if (playerBackend_ && playerBackend_->active()) {
                    playerBackend_->setCameraLook(renderer_.cameraLookDirection());
                    playerBackend_->tick(input, dt);
                    playerBackend_->syncBody(world_.marioBody());
                    world_.advanceFrame();
                } else {
                    (void)input;
                    (void)dt;
                }
                if (debugState_.recording) {
                    replay_.record(world_.frame(), input, world_.marioBody());
                }
            });
            debugState_.frameStepRequested = false;
        }
        updatePoof(static_cast<float>(elapsed));
        refreshMapDebugState();
        audioAccumulator_ += elapsed;
        while (audioAccumulator_ >= 1.0 / 30.0) {
            pumpAudio();
            audioAccumulator_ -= 1.0 / 30.0;
        }

        buildDebugDraw();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        debugUi_.draw(world_, tweaks_, replay_, ghost_, debugState_);
        drawRomOverlay();
        ImGui::Render();

        int width = 0;
        int height = 0;
        SDL_GetWindowSize(window_, &width, &height);
        renderer_.beginFrame(width, height);
        rendering::MapRenderOptions mapOptions;
        mapOptions.solid = debugState_.drawSolidMap;
        mapOptions.wireframe = debugState_.drawWireframeCollision;
        mapOptions.surfaceColors = debugState_.drawSurfaceColors;
        mapOptions.triangleNormals = debugState_.drawTriangleNormals;
        renderer_.drawMap(currentMap_, mapOptions);
        const assets::Mesh* marioMesh = playerBackend_ && playerBackend_->active()
            ? playerBackend_->mesh()
            : (runtimeAssets_.hasMarioMesh() ? &runtimeAssets_.marioMesh() : nullptr);
        if (playerBackend_ && playerBackend_->active()) {
            renderer_.drawMario(world_.marioBody(), marioMesh);
        }
        renderer_.drawDebugLines(debugRenderer_);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        renderer_.endFrame();
    }

    return 0;
}

void SandboxApp::shutdown()
{
    shutdownAudio();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    renderer_.shutdown();
    if (playerBackend_) {
        playerBackend_->shutdown();
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

mario::MarioInput SandboxApp::pollInput(bool& quit)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            quit = true;
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            quit = true;
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    mario::MarioInput input;
    input.stick.x = (keys[SDL_SCANCODE_D] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_A] ? 1.0f : 0.0f);
    input.stick.y = (keys[SDL_SCANCODE_W] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_S] ? 1.0f : 0.0f);
    if (glm::length(input.stick) > 1.0f) {
        input.stick = glm::normalize(input.stick);
    }
    input.jumpPressed = keys[SDL_SCANCODE_SPACE] != 0;
    input.crouchHeld = keys[SDL_SCANCODE_LCTRL] != 0 || keys[SDL_SCANCODE_RCTRL] != 0;
    input.attackPressed = keys[SDL_SCANCODE_LSHIFT] != 0 || keys[SDL_SCANCODE_RSHIFT] != 0;

    cameraOrbitInput_ = (keys[SDL_SCANCODE_E] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_Q] ? 1.0f : 0.0f);
    cameraOrbitInput_ += (keys[SDL_SCANCODE_RIGHT] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_LEFT] ? 1.0f : 0.0f);
    cameraOrbitInput_ = std::clamp(cameraOrbitInput_, -1.0f, 1.0f);
    cameraZoomInput_ = (keys[SDL_SCANCODE_DOWN] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_UP] ? 1.0f : 0.0f);
    return input;
}

void SandboxApp::registerConsoleCommands()
{
    debugUi_.console().registerCommand("reset", "reset simulation and replay", [this](const std::vector<std::string>&) {
        world_.reset();
        applyActiveMap();
        replay_.clear();
        return debug::ConsoleResult { true, "simulation reset" };
    });
    debugUi_.console().registerCommand("save_replay", "save replay to last_replay.json", [this](const std::vector<std::string>&) {
        return debug::ConsoleResult { replay_.save("last_replay.json"), "saved last_replay.json" };
    });
    debugUi_.console().registerCommand("load_ghost", "load last_replay.json as ghost", [this](const std::vector<std::string>&) {
        return debug::ConsoleResult { ghost_.load("last_replay.json"), "loaded last_replay.json as ghost" };
    });
    debugUi_.console().registerCommand("get", "get a tweak variable: get <name>", [this](const std::vector<std::string>& args) {
        if (args.size() != 2) {
            return debug::ConsoleResult { false, "usage: get <name>" };
        }
        const auto value = tweaks_.number(args[1]);
        if (!value) {
            return debug::ConsoleResult { false, "unknown tweak: " + args[1] };
        }
        return debug::ConsoleResult { true, args[1] + " = " + std::to_string(*value) };
    });
    debugUi_.console().registerCommand("set", "set a tweak variable: set <name> <value>", [this](const std::vector<std::string>& args) {
        if (args.size() != 3) {
            return debug::ConsoleResult { false, "usage: set <name> <value>" };
        }
        try {
            tweaks_.setNumber(args[1], std::stod(args[2]));
            tweaks_.applyTo(world_.marioController().tuning());
        } catch (const std::exception&) {
            return debug::ConsoleResult { false, "invalid numeric value: " + args[2] };
        }
        return debug::ConsoleResult { true, args[1] + " = " + args[2] };
    });
    debugUi_.console().registerCommand("pause", "toggle pause", [this](const std::vector<std::string>&) {
        debugState_.paused = !debugState_.paused;
        return debug::ConsoleResult { true, debugState_.paused ? "paused" : "running" };
    });
    debugUi_.console().registerCommand("backend", "show active Mario backend", [this](const std::vector<std::string>&) {
        return debug::ConsoleResult { true, playerBackend_ ? playerBackend_->status() : "no ROM avatar backend active" };
    });
    debugUi_.console().registerCommand("load_map", "load a JSON map: load_map <path>", [this](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            return debug::ConsoleResult { false, "usage: load_map <path>" };
        }
        std::string path = args[1];
        for (std::size_t i = 2; i < args.size(); ++i) {
            path += " " + args[i];
        }
        const bool ok = loadMap(path);
        return debug::ConsoleResult { ok, ok ? "loaded map: " + currentMap_.name() : "failed to load map: " + path };
    });
    debugUi_.console().registerCommand("reload_map", "reload the active map file", [this](const std::vector<std::string>&) {
        if (currentMapPath_.empty()) {
            loadDefaultMap();
            return debug::ConsoleResult { true, "reloaded built-in movement lab" };
        }
        const bool ok = loadMap(currentMapPath_);
        return debug::ConsoleResult { ok, ok ? "reloaded map: " + currentMap_.name() : "failed to reload map" };
    });
    debugUi_.console().registerCommand("map_info", "show active map information", [this](const std::vector<std::string>&) {
        return debug::ConsoleResult { true,
            currentMap_.name() + " | meshes=" + std::to_string(currentMap_.meshes().size())
                + " triangles=" + std::to_string(currentMap_.triangleCount()) };
    });
}

void SandboxApp::buildDebugDraw()
{
    debugRenderer_.clear();
    const mario::MarioBody& body = world_.marioBody();

    if (debugState_.drawVelocity) {
        debugRenderer_.vector(body.position + glm::vec3(0.0f, 5.0f, 0.0f), body.velocity, { 0.1f, 0.7f, 1.0f }, 1.0f);
    }
    if (debugState_.drawNormals) {
        debugRenderer_.normal(body.position, body.floorNormal, { 1.0f, 0.85f, 0.15f }, 12.0f);
        if (body.touchedWall) {
            debugRenderer_.normal(body.position, body.wallNormal, { 1.0f, 0.2f, 0.8f }, 12.0f);
        }
    }

    for (const SmokeParticle& smoke : smoke_) {
        const float t = smoke.lifetime > 0.0f ? std::clamp(smoke.age / smoke.lifetime, 0.0f, 1.0f) : 1.0f;
        const glm::vec3 color = glm::vec3(0.72f, 0.72f, 0.68f) * (1.0f - t * 0.45f);
        const float radius = smoke.size * (1.0f + t * 2.0f);
        debugRenderer_.line(smoke.position + glm::vec3(-radius, 0.0f, 0.0f), smoke.position + glm::vec3(radius, 0.0f, 0.0f), color);
        debugRenderer_.line(smoke.position + glm::vec3(0.0f, -radius, 0.0f), smoke.position + glm::vec3(0.0f, radius, 0.0f), color);
        debugRenderer_.line(smoke.position + glm::vec3(0.0f, 0.0f, -radius), smoke.position + glm::vec3(0.0f, 0.0f, radius), color);
    }
}

void SandboxApp::updateRomRuntime()
{
    const auto& snapshot = romManager_.update();
    debugState_.romWatcherStatus = assets::romManagerModeName(snapshot.mode);
    debugState_.romStatus = snapshot.message;
    debugState_.romWarning = snapshot.multipleRoms ? "Multiple ROM files present; using the first stable supported ROM." : "";

    if (snapshot.loaded) {
        debugState_.currentRomFilename = snapshot.loaded->filename;
        debugState_.currentRomHash = snapshot.loaded->sha1;
        debugState_.currentRomGame = assets::romGameName(snapshot.loaded->game);
    } else if (snapshot.mode == assets::RomManagerMode::Unsupported) {
        debugState_.currentRomFilename = snapshot.candidatePath.filename().string();
        debugState_.currentRomHash = "unknown";
        debugState_.currentRomGame = "unsupported";
    } else {
        debugState_.currentRomFilename = "none";
        debugState_.currentRomHash = "none";
        debugState_.currentRomGame = "none";
    }

    if (snapshot.activeFileMissing) {
        despawnBackend(true);
        return;
    }

    if (snapshot.loaded && activeRomSha1_ != snapshot.loaded->sha1) {
        spawnBackend(*snapshot.loaded);
    }

    debugState_.backendStatus = playerBackend_ ? playerBackend_->status() : "no ROM avatar backend active";
}

void SandboxApp::spawnBackend(const assets::RomImage& rom)
{
    despawnBackend(false);

    if (rom.game == assets::RomGame::SuperMario64) {
        auto backend = std::make_unique<mario::LibSm64Backend>();
        if (!backend->initialize(findLibSm64Dll(), rom, world_.collisionWorld())) {
            util::logWarn(backend->status());
            debugState_.backendStatus = backend->status();
            return;
        }
        playerBackend_ = std::move(backend);
    } else if (rom.game == assets::RomGame::MarioKart64) {
        auto backend = std::make_unique<mario::MarioKartBackend>();
        if (!backend->initialize(rom, world_.collisionWorld())) {
            util::logWarn(backend->status());
            debugState_.backendStatus = backend->status();
            return;
        }
        playerBackend_ = std::move(backend);
    }

    if (!playerBackend_) {
        debugState_.backendStatus = "unsupported ROM backend";
        return;
    }

    activeRomSha1_ = rom.sha1;
    world_.reset();
    applyActiveMap();
    playerBackend_->syncBody(world_.marioBody());

    if (const auto* texture = playerBackend_->textureRgba(); texture && !texture->empty()) {
        renderer_.setMarioTexture(*texture, playerBackend_->textureWidth(), playerBackend_->textureHeight());
    } else {
        renderer_.clearMarioTexture();
    }
    initializeAudio();
    util::logInfo("Spawned ROM avatar backend: ", playerBackend_->status());
}

void SandboxApp::despawnBackend(bool poof)
{
    const bool hadBackend = playerBackend_ && playerBackend_->active();
    const glm::vec3 poofPosition = world_.marioBody().position + glm::vec3(0.0f, 32.0f, 0.0f);
    shutdownAudio();
    if (playerBackend_) {
        playerBackend_->shutdown();
        playerBackend_.reset();
    }
    activeRomSha1_.clear();
    renderer_.clearMarioTexture();
    world_.marioBody() = mario::MarioBody {};
    replay_.clear();
    if (poof && hadBackend) {
        spawnPoof(poofPosition);
    }
}

void SandboxApp::spawnPoof(glm::vec3 position)
{
    smoke_.clear();
    for (int i = 0; i < 18; ++i) {
        const float angle = static_cast<float>(i) * 0.34906585f;
        const float ring = i % 3 == 0 ? 70.0f : 42.0f;
        SmokeParticle particle;
        particle.position = position;
        particle.velocity = { std::sin(angle) * ring, 35.0f + static_cast<float>(i % 5) * 14.0f, std::cos(angle) * ring };
        particle.lifetime = 0.85f + static_cast<float>(i % 4) * 0.12f;
        particle.size = 12.0f + static_cast<float>(i % 4) * 4.0f;
        smoke_.push_back(particle);
    }
}

void SandboxApp::updatePoof(float dt)
{
    for (SmokeParticle& particle : smoke_) {
        particle.age += dt;
        particle.position += particle.velocity * dt;
        particle.velocity *= std::pow(0.35f, dt);
    }
    smoke_.erase(std::remove_if(smoke_.begin(), smoke_.end(), [](const SmokeParticle& particle) {
        return particle.age >= particle.lifetime;
    }), smoke_.end());
}

void SandboxApp::drawRomOverlay()
{
    if (playerBackend_ && playerBackend_->active()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.42f);
    const std::string message = debugState_.romStatus.empty() ? "Insert a ROM" : debugState_.romStatus;
    const ImVec2 size = ImGui::CalcTextSize(message.c_str());
    ImGui::SetNextWindowPos(ImVec2(center.x - size.x * 0.5f - 28.0f, center.y - 24.0f));
    ImGui::SetNextWindowBgAlpha(0.72f);
    ImGui::Begin("ROM Prompt", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::Text("%s", message.c_str());
    ImGui::End();
}

bool SandboxApp::initializeAudio()
{
    if (!playerBackend_ || !playerBackend_->initializeAudio()) {
        util::logWarn("backend audio initialization failed");
        return false;
    }

    SDL_AudioSpec want {};
    want.freq = 32000;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 512;
    want.callback = nullptr;

    SDL_AudioSpec have {};
    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (audioDevice_ == 0) {
        util::logWarn("SDL_OpenAudioDevice failed: ", SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(audioDevice_, 0);
    util::logInfo("SM64 audio initialized at ", have.freq, " Hz");
    return true;
}

void SandboxApp::pumpAudio()
{
    if (audioDevice_ == 0 || !playerBackend_ || !playerBackend_->audioActive()) {
        return;
    }

    const std::uint32_t queuedSamples = SDL_GetQueuedAudioSize(audioDevice_) / 4U;
    if (queuedSamples >= 6000U) {
        return;
    }

    std::vector<std::int16_t> samples;
    const std::uint32_t generatedSamples = playerBackend_->tickAudio(queuedSamples, 1100U, samples);
    if (generatedSamples == 0 || samples.empty()) {
        return;
    }

    SDL_QueueAudio(audioDevice_, samples.data(), static_cast<Uint32>(samples.size() * sizeof(std::int16_t)));
}

void SandboxApp::shutdownAudio()
{
    if (audioDevice_ != 0) {
        SDL_ClearQueuedAudio(audioDevice_);
        SDL_CloseAudioDevice(audioDevice_);
        audioDevice_ = 0;
    }
}

bool SandboxApp::loadMap(const std::filesystem::path& path)
{
    std::string error;
    auto map = assets::TestMap::loadJson(path, error);
    if (!map) {
        util::logWarn("Map load failed: ", error);
        return false;
    }

    currentMap_ = std::move(*map);
    currentMapPath_ = path;
    applyActiveMap();
    refreshMapDebugState();
    util::logInfo("Loaded map ", currentMap_.name(), " (", currentMap_.triangleCount(), " triangles)");
    return true;
}

void SandboxApp::loadDefaultMap()
{
    const auto mapPath = findDefaultMapPath();
    if (!loadMap(mapPath)) {
        currentMap_ = assets::TestMap::builtInMovementLab();
        currentMapPath_.clear();
        applyActiveMap();
        refreshMapDebugState();
        util::logWarn("Using built-in movement lab map");
    }
}

void SandboxApp::applyActiveMap()
{
    currentMap_.applyTo(world_.collisionWorld());
    if (playerBackend_ && playerBackend_->active()) {
        playerBackend_->reloadSurfaces(world_.collisionWorld());
    }
}

void SandboxApp::refreshMapDebugState()
{
    debugState_.currentMapName = currentMap_.name();
    debugState_.currentFloorSurface = "none";

    const auto floor = world_.collisionWorld().findFloor(world_.marioBody().position, 50.0f);
    if (floor && floor->surface) {
        debugState_.currentFloorSurface = assets::surfaceTypeToString(floor->surface->type);
    }
}

} // namespace sm64ps
