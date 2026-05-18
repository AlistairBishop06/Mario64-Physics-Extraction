#include "util/Config.h"

#include "util/Log.h"

#include <fstream>

namespace sm64ps::util {

bool Config::load(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file) {
        logWarn("Config not found: ", path.string());
        return false;
    }

    try {
        file >> data_;
    } catch (const std::exception& error) {
        logError("Failed to parse config ", path.string(), ": ", error.what());
        return false;
    }

    return true;
}

bool Config::save(const std::filesystem::path& path) const
{
    std::ofstream file(path);
    if (!file) {
        logError("Failed to open config for writing: ", path.string());
        return false;
    }

    file << data_.dump(2) << '\n';
    return true;
}

} // namespace sm64ps::util

