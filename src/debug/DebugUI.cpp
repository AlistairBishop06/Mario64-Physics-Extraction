#include "debug/DebugUI.h"

#include "mario/MarioMath.h"

#include <imgui.h>

#include <cstdio>

namespace sm64ps::debug {

namespace {

void vec3Text(const char* label, const glm::vec3& value)
{
    ImGui::Text("%s: %.3f, %.3f, %.3f", label, value.x, value.y, value.z);
}

void surfaceLegend()
{
    const surfaces::SurfaceType surfaces[] = {
        surfaces::SurfaceType::Default,
        surfaces::SurfaceType::Slippery,
        surfaces::SurfaceType::VerySlippery,
        surfaces::SurfaceType::NotSlippery,
        surfaces::SurfaceType::WallKickable,
        surfaces::SurfaceType::LedgeGrab,
        surfaces::SurfaceType::Lava,
        surfaces::SurfaceType::Quicksand,
    };

    for (const auto surface : surfaces) {
        const glm::vec3 color = assets::defaultSurfaceColor(surface);
        ImGui::ColorButton(assets::surfaceTypeToString(surface).c_str(), ImVec4(color.x, color.y, color.z, 1.0f),
            ImGuiColorEditFlags_NoTooltip, ImVec2(14.0f, 14.0f));
        ImGui::SameLine();
        ImGui::Text("%s", assets::surfaceTypeToString(surface).c_str());
    }
}

} // namespace

DebugUI::DebugUI()
{
    console_.registerCommand("help", "show available commands", [this](const std::vector<std::string>&) {
        std::string output;
        for (const std::string& line : console_.helpLines()) {
            output += line + "\n";
        }
        return ConsoleResult { true, output };
    });
}

void DebugUI::draw(physics::PhysicsWorld& world, TweakVars& tweaks, replay::ReplayTrack& replay,
    const replay::ReplayTrack& ghost, DebugUiState& state)
{
    const mario::MarioBody& body = world.marioBody();

    ImGui::Begin("SM64 Physics Sandbox");
    ImGui::Text("Frame: %llu", static_cast<unsigned long long>(world.frame()));
    ImGui::Text("Map: %s", state.currentMapName.c_str());
    if (!state.backendStatus.empty()) {
        ImGui::TextWrapped("Backend: %s", state.backendStatus.c_str());
    }
    ImGui::Text("ROM watcher: %s", state.romWatcherStatus.c_str());
    ImGui::TextWrapped("ROM status: %s", state.romStatus.c_str());
    ImGui::Text("ROM: %s", state.currentRomFilename.c_str());
    ImGui::Text("Game: %s", state.currentRomGame.c_str());
    ImGui::Text("SHA-1: %s", state.currentRomHash.c_str());
    if (!state.romWarning.empty()) {
        ImGui::TextWrapped("Warning: %s", state.romWarning.c_str());
    }
    ImGui::Text("Action: %s", mario::actionName(body.action).data());
    vec3Text("Position", body.position);
    vec3Text("Velocity", body.velocity);
    ImGui::Text("Face yaw: 0x%04x", body.faceYaw);
    ImGui::Text("Floor angle: %.2f deg", mario::floorSlopeDegrees(body.floorNormal));
    ImGui::Text("Floor surface: %s", state.currentFloorSurface.c_str());

    ImGui::Separator();
    ImGui::Checkbox("Pause", &state.paused);
    if (ImGui::Button("Frame Step")) {
        state.frameStepRequested = true;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Record", &state.recording);

    ImGui::Separator();
    ImGui::Checkbox("Velocity vector", &state.drawVelocity);
    ImGui::Checkbox("Collision normals", &state.drawNormals);
    ImGui::Checkbox("Solid map", &state.drawSolidMap);
    ImGui::Checkbox("Wireframe collision", &state.drawWireframeCollision);
    ImGui::Checkbox("Surface colors", &state.drawSurfaceColors);
    ImGui::Checkbox("Triangle normals", &state.drawTriangleNormals);

    if (ImGui::CollapsingHeader("Surface Legend")) {
        surfaceLegend();
    }

    if (ghost.empty()) {
        ImGui::Text("Ghost: none loaded");
    } else {
        const replay::GhostDiff diff = replay::compareGhost(ghost, world.frame(), body);
        if (diff.available) {
            vec3Text("Ghost position delta", diff.positionDelta);
            vec3Text("Ghost velocity delta", diff.velocityDelta);
        }
    }
    ImGui::End();

    ImGui::Begin("Runtime Tweaks");
    bool changed = false;
    for (const auto& [name, value] : tweaks.numbers()) {
        float editValue = static_cast<float>(value);
        if (ImGui::DragFloat(name.c_str(), &editValue, 0.05f)) {
            tweaks.setNumber(name, editValue);
            changed = true;
        }
    }
    if (changed) {
        tweaks.applyTo(world.marioController().tuning());
    }
    ImGui::End();

    ImGui::Begin("Replay");
    ImGui::Text("Recorded samples: %zu", replay.samples().size());
    int maxFrame = replay.empty() ? 0 : static_cast<int>(replay.samples().back().frame);
    ImGui::SliderInt("Timeline", &state.replayFrame, 0, maxFrame);
    if (ImGui::Button("Save replay")) {
        replay.save("last_replay.json");
    }
    ImGui::SameLine();
    if (ImGui::Button("Load as ghost")) {
        consoleOutput_ = "Load-as-ghost is wired at app level in future UI pass.";
    }
    ImGui::End();

    drawConsole();
}

void DebugUI::drawConsole()
{
    ImGui::Begin("Developer Console");
    ImGui::TextWrapped("%s", consoleOutput_.c_str());
    if (ImGui::InputText("Command", commandBuffer_.data(), commandBuffer_.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
        const ConsoleResult result = console_.execute(commandBuffer_.data());
        consoleOutput_ = result.message;
        commandBuffer_.fill('\0');
    }
    ImGui::End();
}

} // namespace sm64ps::debug
