#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sm64ps::util {

class Sha1 {
public:
    Sha1();

    void update(const std::uint8_t* data, std::size_t length);
    void update(const std::vector<std::uint8_t>& data);
    std::array<std::uint8_t, 20> final();

    static std::optional<std::string> fileHexDigest(const std::filesystem::path& path);
    static std::string toHex(const std::array<std::uint8_t, 20>& digest);

private:
    void processBlock(const std::uint8_t* block);

    std::uint64_t totalBytes_ = 0;
    std::array<std::uint32_t, 5> state_ {};
    std::array<std::uint8_t, 64> buffer_ {};
    std::size_t bufferSize_ = 0;
    bool finalized_ = false;
};

} // namespace sm64ps::util

