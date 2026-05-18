#include "SandboxApp.h"

#include "mario/MarioMath.h"
#include "util/Config.h"
#include "util/Log.h"

#include <SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <string>

#include <glm/geometric.hpp>

namespace sm64ps {

bool SandboxApp::initialize()
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
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
    if (config.load("configs/default.json")) {
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

        const bool shouldStep = !debugState_.paused || debugState_.frameStepRequested;
        if (shouldStep) {
            timestep_.advance(elapsed, [&](float dt) {
                world_.step(input, dt);
                if (debugState_.recording) {
                    replay_.record(world_.frame(), input, world_.marioBody());
                }
            });
            debugState_.frameStepRequested = false;
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
        renderer_.drawMario(world_.marioBody());
        renderer_.drawDebugLines(debugRenderer_);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        renderer_.endFrame();
    }

    return 0;
}

void SandboxApp::shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    renderer_.shutdown();
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

} // namespace sm64ps
