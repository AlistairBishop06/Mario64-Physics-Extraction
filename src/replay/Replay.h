#pragma once

#include "mario/MarioState.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include <nlohmann/json.hpp>

namespace sm64ps::replay {

struct ReplaySample {
    std::uint64_t frame = 0;
    mario::MarioInput input;
    mario::MarioBody body;
};

class ReplayTrack {
public:
    void clear();
    void record(std::uint64_t frame, const mario::MarioInput& input, const mario::MarioBody& body);
    std::optional<ReplaySample> sampleAtFrame(std::uint64_t frame) const;

    bool save(const std::filesystem::path& path) const;
    bool load(const std::filesystem::path& path);

    const std::vector<ReplaySample>& samples() const { return samples_; }
    bool empty() const { return samples_.empty(); }

private:
    std::vector<ReplaySample> samples_;
};

struct GhostDiff {
    glm::vec3 positionDelta { 0.0f };
    glm::vec3 velocityDelta { 0.0f };
    bool available = false;
};

GhostDiff compareGhost(const ReplayTrack& track, std::uint64_t frame, const mario::MarioBody& body);

void to_json(nlohmann::json& json, const ReplaySample& sample);
void from_json(const nlohmann::json& json, ReplaySample& sample);

} // namespace sm64ps::replay

