#include "SandboxApp.h"

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

std::filesystem::path findLibSm64Dll()
{
    std::vector<std::filesystem::path> candidates;
    for (const auto& root : searchRoots()) {
        candidates.push_back(root / "sm64.dll");
        candidates.push_back(root / "third_party" / "libsm64" / "dist" / "sm64.dll");
    }
    return findFirstExisting(candidates).value_or(mario::LibSm64Backend::findDefaultLibrary());
}

std::filesystem::path findRomPath()
{
    for (const auto& root : searchRoots()) {
        const auto romDirectory = std::filesystem::weakly_canonical(root / "roms");
        if (!std::filesystem::exists(romDirectory)) {
            continue;
        }

        for (const auto& entry : std::filesystem::directory_iterator(romDirectory)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const std::string extension = entry.path().extension().string();
            if (extension == ".z64" || extension == ".n64" || extension == ".v64") {
                return entry.path();
            }
        }
    }

    return mario::LibSm64Backend::findDefaultRom();
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
    const auto libSm64Path = findLibSm64Dll();
    const auto libSm64Rom = findRomPath();
    if (libSm64_.initialize(libSm64Path, libSm64Rom, world_.collisionWorld())) {
        renderer_.setMarioTexture(libSm64_.textureRgba(), 64 * 11, 64);
        initializeAudio();
    } else {
        util::logWarn(libSm64_.status());
    }
    debugState_.backendStatus = libSm64_.status();
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
        renderer_.updateLakituCamera(world_.marioBody(), cameraOrbitInput_, cameraZoomInput_);

        const bool shouldStep = !debugState_.paused || debugState_.frameStepRequested;
        if (shouldStep) {
            timestep_.advance(elapsed, [&](float dt) {
                if (libSm64_.active()) {
                    (void)dt;
                    libSm64_.setCameraLook(renderer_.cameraLookDirection());
                    libSm64_.tick(input);
                    libSm64_.syncBody(world_.marioBody());
                    world_.advanceFrame();
                } else {
                    world_.step(input, dt);
                }
                if (debugState_.recording) {
                    replay_.record(world_.frame(), input, world_.marioBody());
                }
            });
            debugState_.frameStepRequested = false;
        }
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
        ImGui::Render();

        int width = 0;
        int height = 0;
        SDL_GetWindowSize(window_, &width, &height);
        renderer_.beginFrame(width, height);
        if (debugState_.drawCollision) {
            renderer_.drawCollision(world_.collisionWorld());
        }
        const assets::Mesh* marioMesh = libSm64_.active()
            ? &libSm64_.mesh()
            : (runtimeAssets_.hasMarioMesh() ? &runtimeAssets_.marioMesh() : nullptr);
        renderer_.drawMario(world_.marioBody(), marioMesh);
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
    libSm64_.shutdown();
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
        return debug::ConsoleResult { true, libSm64_.status() };
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
}

bool SandboxApp::initializeAudio()
{
    if (!libSm64_.initializeAudio()) {
        util::logWarn("libsm64 audio initialization failed");
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
    if (audioDevice_ == 0 || !libSm64_.audioActive()) {
        return;
    }

    const std::uint32_t queuedSamples = SDL_GetQueuedAudioSize(audioDevice_) / 4U;
    if (queuedSamples >= 6000U) {
        return;
    }

    std::vector<std::int16_t> samples;
    const std::uint32_t generatedSamples = libSm64_.tickAudio(queuedSamples, 1100U, samples);
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

} // namespace sm64ps
