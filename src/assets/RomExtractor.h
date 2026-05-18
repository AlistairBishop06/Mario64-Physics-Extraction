#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sm64ps::assets {

struct KnownRom {
    std::string label;
    std::string sha1;
    std::string region;
    bool supported = false;
};

struct RomVerification {
    bool readable = false;
    bool recognized = false;
    std::string sha1;
    std::optional<KnownRom> knownRom;
};

class RomExtractor {
public:
    RomVerification verify(const std::filesystem::path& romPath) const;
    bool extractMovementData(const std::filesystem::path& romPath, const std::filesystem::path& outputDirectory) const;
    const std::vector<KnownRom>& knownRoms() const { return knownRoms_; }

private:
    std::vector<KnownRom> knownRoms_ {
        { "Super Mario 64 (U) [!]", "9bef1128717f958171a4afac3ed78ee2bb4e86ce", "US", true },
        { "Super Mario 64 (J) [!]", "8a20a5c83d6ceb0f0506cfc9fa20d8f438cafe51", "JP", true },
        { "Super Mario 64 (E) [!]", "4ac5721683d0e0b6bbb561b58a71740845dceea9", "EU", true }
    };
};

} // namespace sm64ps::assets

