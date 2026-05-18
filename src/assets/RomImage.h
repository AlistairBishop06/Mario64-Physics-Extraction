#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sm64ps::assets {

enum class RomGame {
    Unknown,
    SuperMario64,
    MarioKart64
};

struct RomImage {
    std::filesystem::path path;
    std::string filename;
    std::string sha1;
    std::string internalName;
    RomGame game = RomGame::Unknown;
    bool supported = false;
    std::vector<std::uint8_t> bytes;
};

const char* romGameName(RomGame game);
RomImage classifyRom(std::filesystem::path path, std::vector<std::uint8_t> bytes);

} // namespace sm64ps::assets
