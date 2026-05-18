#include "assets/RomManager.h"

#include <algorithm>
#include <fstream>

namespace sm64ps::assets {

namespace {

bool isRomExtension(const std::filesystem::path& path)
{
    const std::string extension = path.extension().string();
    return extension == ".z64" || extension == ".n64" || extension == ".v64"
        || extension == ".Z64" || extension == ".N64" || extension == ".V64";
}

std::string shortHash(const std::string& sha1)
{
    return sha1.size() > 8 ? sha1.substr(0, 8) : sha1;
}

} // namespace

RomManager::RomManager(std::filesystem::path romDirectory)
    : romDirectory_(std::move(romDirectory))
{
}

const RomManagerSnapshot& RomManager::update()
{
    snapshot_.activeFileMissing = false;
    snapshot_.multipleRoms = false;
    snapshot_.romCount = 0;

    const auto files = romFiles();
    snapshot_.romCount = files.size();
    snapshot_.multipleRoms = files.size() > 1;

    if (snapshot_.loaded) {
        const bool loadedStillPresent = std::filesystem::exists(snapshot_.loaded->path);
        if (loadedStillPresent) {
            snapshot_.mode = RomManagerMode::Loaded;
            snapshot_.message = std::string("Loaded ") + romGameName(snapshot_.loaded->game) + " (" + snapshot_.loaded->filename + ")";
            return snapshot_;
        }

        snapshot_.activeFileMissing = true;
        snapshot_.loaded.reset();
        stableCandidate_.reset();
        snapshot_.mode = RomManagerMode::Empty;
        snapshot_.message = "Insert a new ROM";
        return snapshot_;
    }

    if (files.empty()) {
        stableCandidate_.reset();
        snapshot_.mode = RomManagerMode::Empty;
        snapshot_.candidatePath.clear();
        snapshot_.candidateSize = 0;
        snapshot_.message = "Insert a ROM";
        return snapshot_;
    }

    const auto path = files.front();
    const std::uintmax_t size = std::filesystem::file_size(path);
    const auto writeTime = std::filesystem::last_write_time(path);
    snapshot_.candidatePath = path;
    snapshot_.candidateSize = size;

    if (!stableEnough(path, size, writeTime)) {
        snapshot_.mode = RomManagerMode::WaitingForStableFile;
        snapshot_.message = "Reading ROM...";
        return snapshot_;
    }

    std::string error;
    auto image = readCandidate(path, error);
    if (!image) {
        snapshot_.mode = RomManagerMode::Error;
        snapshot_.message = error.empty() ? "Could not read ROM" : error;
        return snapshot_;
    }

    if (!image->supported) {
        snapshot_.mode = RomManagerMode::Unsupported;
        snapshot_.message = "Unsupported ROM";
        return snapshot_;
    }

    snapshot_.mode = RomManagerMode::Loaded;
    snapshot_.message = std::string("Loaded ") + romGameName(image->game) + " (" + shortHash(image->sha1) + ")";
    snapshot_.loaded = std::move(*image);
    return snapshot_;
}

void RomManager::forgetLoadedRom()
{
    snapshot_.loaded.reset();
    stableCandidate_.reset();
}

std::vector<std::filesystem::path> RomManager::romFiles() const
{
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(romDirectory_)) {
        std::filesystem::create_directories(romDirectory_);
        return files;
    }

    for (const auto& entry : std::filesystem::directory_iterator(romDirectory_)) {
        if (entry.is_regular_file() && isRomExtension(entry.path())) {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

bool RomManager::stableEnough(const std::filesystem::path& path, std::uintmax_t size,
    std::filesystem::file_time_type writeTime)
{
    const auto now = std::chrono::steady_clock::now();
    if (!stableCandidate_ || stableCandidate_->path != path || stableCandidate_->size != size
        || stableCandidate_->writeTime != writeTime) {
        stableCandidate_ = StableCandidate { path, size, writeTime, now };
        return false;
    }

    return now - stableCandidate_->firstSeen >= std::chrono::milliseconds(750);
}

std::optional<RomImage> RomManager::readCandidate(const std::filesystem::path& path, std::string& error) const
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "Could not open ROM";
        return std::nullopt;
    }

    std::vector<std::uint8_t> bytes { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    file.close();

    if (bytes.empty()) {
        error = "ROM is empty";
        return std::nullopt;
    }

    return classifyRom(path, std::move(bytes));
}

const char* romManagerModeName(RomManagerMode mode)
{
    switch (mode) {
    case RomManagerMode::Empty: return "empty";
    case RomManagerMode::WaitingForStableFile: return "waiting";
    case RomManagerMode::Loaded: return "loaded";
    case RomManagerMode::Unsupported: return "unsupported";
    case RomManagerMode::Error: return "error";
    }
    return "unknown";
}

} // namespace sm64ps::assets
