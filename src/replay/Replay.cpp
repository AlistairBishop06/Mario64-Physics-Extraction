#include "replay/Replay.h"

#include "util/Log.h"

#include <algorithm>
#include <fstream>

namespace sm64ps::replay {

namespace {

nlohmann::json vec2ToJson(const glm::vec2& value)
{
    return { value.x, value.y };
}

nlohmann::json vec3ToJson(const glm::vec3& value)
{
    return { value.x, value.y, value.z };
}

glm::vec2 vec2FromJson(const nlohmann::json& json)
{
    return { json.at(0).get<float>(), json.at(1).get<float>() };
}

glm::vec3 vec3FromJson(const nlohmann::json& json)
{
    return { json.at(0).get<float>(), json.at(1).get<float>(), json.at(2).get<float>() };
}

} // namespace

void ReplayTrack::clear()
{
    samples_.clear();
}

void ReplayTrack::record(std::uint64_t frame, const mario::MarioInput& input, const mario::MarioBody& body)
{
    samples_.push_back(ReplaySample { frame, input, body });
}

std::optional<ReplaySample> ReplayTrack::sampleAtFrame(std::uint64_t frame) const
{
    const auto iter = std::lower_bound(samples_.begin(), samples_.end(), frame,
        [](const ReplaySample& sample, std::uint64_t needle) { return sample.frame < needle; });
    if (iter == samples_.end() || iter->frame != frame) {
        return std::nullopt;
    }
    return *iter;
}

bool ReplayTrack::save(const std::filesystem::path& path) const
{
    std::ofstream file(path);
    if (!file) {
        util::logError("Failed to write replay: ", path.string());
        return false;
    }

    nlohmann::json json;
    json["version"] = 1;
    json["samples"] = samples_;
    file << json.dump(2) << '\n';
    return true;
}

bool ReplayTrack::load(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file) {
        util::logError("Failed to read replay: ", path.string());
        return false;
    }

    nlohmann::json json;
    file >> json;
    samples_ = json.at("samples").get<std::vector<ReplaySample>>();
    return true;
}

GhostDiff compareGhost(const ReplayTrack& track, std::uint64_t frame, const mario::MarioBody& body)
{
    const auto sample = track.sampleAtFrame(frame);
    if (!sample) {
        return {};
    }

    return {
        body.position - sample->body.position,
        body.velocity - sample->body.velocity,
        true
    };
}

void to_json(nlohmann::json& json, const ReplaySample& sample)
{
    json = {
        { "frame", sample.frame },
        { "input", {
            { "stick", vec2ToJson(sample.input.stick) },
            { "jumpPressed", sample.input.jumpPressed },
            { "crouchHeld", sample.input.crouchHeld },
            { "attackPressed", sample.input.attackPressed },
        } },
        { "body", {
            { "position", vec3ToJson(sample.body.position) },
            { "velocity", vec3ToJson(sample.body.velocity) },
            { "floorNormal", vec3ToJson(sample.body.floorNormal) },
            { "wallNormal", vec3ToJson(sample.body.wallNormal) },
            { "faceYaw", sample.body.faceYaw },
            { "intendedYaw", sample.body.intendedYaw },
            { "action", static_cast<int>(sample.body.action) },
            { "grounded", sample.body.grounded },
            { "touchedWall", sample.body.touchedWall },
            { "touchedCeiling", sample.body.touchedCeiling },
            { "actionTimer", sample.body.actionTimer },
            { "jumpCount", static_cast<unsigned int>(sample.body.jumpCount) },
        } }
    };
}

void from_json(const nlohmann::json& json, ReplaySample& sample)
{
    sample.frame = json.at("frame").get<std::uint64_t>();
    const auto& input = json.at("input");
    sample.input.stick = vec2FromJson(input.at("stick"));
    sample.input.jumpPressed = input.at("jumpPressed").get<bool>();
    sample.input.crouchHeld = input.at("crouchHeld").get<bool>();
    sample.input.attackPressed = input.at("attackPressed").get<bool>();

    const auto& body = json.at("body");
    sample.body.position = vec3FromJson(body.at("position"));
    sample.body.velocity = vec3FromJson(body.at("velocity"));
    sample.body.floorNormal = vec3FromJson(body.at("floorNormal"));
    sample.body.wallNormal = vec3FromJson(body.at("wallNormal"));
    sample.body.faceYaw = body.at("faceYaw").get<mario::Angle16>();
    sample.body.intendedYaw = body.at("intendedYaw").get<mario::Angle16>();
    sample.body.action = static_cast<mario::Action>(body.at("action").get<int>());
    sample.body.grounded = body.at("grounded").get<bool>();
    sample.body.touchedWall = body.at("touchedWall").get<bool>();
    sample.body.touchedCeiling = body.at("touchedCeiling").get<bool>();
    sample.body.actionTimer = body.at("actionTimer").get<std::uint32_t>();
    sample.body.jumpCount = static_cast<std::uint8_t>(body.at("jumpCount").get<unsigned int>());
}

} // namespace sm64ps::replay
