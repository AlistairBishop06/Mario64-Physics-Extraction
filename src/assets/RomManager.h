#pragma once

#include "assets/RomImage.h"

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sm64ps::assets {

enum class RomManagerMode {
    Empty,
    WaitingForStableFile,
    Loaded,
    Unsupported,
    Error
};

struct RomManagerSnapshot {
    RomManagerMode mode = RomManagerMode::Empty;
    std::optional<RomImage> loaded;
    std::filesystem::path candidatePath;
    std::uintmax_t candidateSize = 0;
    bool activeFileMissing = false;
    bool multipleRoms = false;
    std::size_t romCount = 0;
    std::string message = "Insert a ROM";
};

class RomManager {
public:
    explicit RomManager(std::filesystem::path romDirectory = "roms");

    const RomManagerSnapshot& update();
    void forgetLoadedRom();
    const RomManagerSnapshot& snapshot() const { return snapshot_; }

private:
    struct StableCandidate {
        std::filesystem::path path;
        std::uintmax_t size = 0;
        std::filesystem::file_time_type writeTime {};
        std::chrono::steady_clock::time_point firstSeen {};
    };

    std::vector<std::filesystem::path> romFiles() const;
    bool stableEnough(const std::filesystem::path& path, std::uintmax_t size,
        std::filesystem::file_time_type writeTime);
    std::optional<RomImage> readCandidate(const std::filesystem::path& path, std::string& error) const;

    std::filesystem::path romDirectory_;
    RomManagerSnapshot snapshot_;
    std::optional<StableCandidate> stableCandidate_;
};

const char* romManagerModeName(RomManagerMode mode);

} // namespace sm64ps::assets
