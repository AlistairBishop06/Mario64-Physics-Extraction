#include "assets/RomImage.h"

#include "util/Sha1.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace sm64ps::assets {

namespace {

std::vector<std::uint8_t> byteswappedToBigEndian(const std::vector<std::uint8_t>& bytes)
{
    std::vector<std::uint8_t> normalized = bytes;
    if (normalized.size() < 4) {
        return normalized;
    }

    const std::array<std::uint8_t, 4> magic {
        normalized[0],
        normalized[1],
        normalized[2],
        normalized[3],
    };

    if (magic == std::array<std::uint8_t, 4> { 0x80, 0x37, 0x12, 0x40 }) {
        return normalized;
    }

    if (magic == std::array<std::uint8_t, 4> { 0x37, 0x80, 0x40, 0x12 }) {
        for (std::size_t i = 0; i + 1 < normalized.size(); i += 2) {
            std::swap(normalized[i], normalized[i + 1]);
        }
        return normalized;
    }

    if (magic == std::array<std::uint8_t, 4> { 0x40, 0x12, 0x37, 0x80 }) {
        for (std::size_t i = 0; i + 3 < normalized.size(); i += 4) {
            std::swap(normalized[i], normalized[i + 3]);
            std::swap(normalized[i + 1], normalized[i + 2]);
        }
    }
    return normalized;
}

std::string headerName(const std::vector<std::uint8_t>& bytes)
{
    if (bytes.size() < 0x34) {
        return {};
    }

    std::string name;
    for (std::size_t i = 0x20; i < 0x34; ++i) {
        const unsigned char c = bytes[i];
        if (c == 0) {
            break;
        }
        name.push_back(std::isprint(c) ? static_cast<char>(c) : ' ');
    }

    while (!name.empty() && name.back() == ' ') {
        name.pop_back();
    }
    return name;
}

std::string uppercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

bool isSm64UsSha1(const std::string& sha1)
{
    return sha1 == "9bef1128717f958171a4afac3ed78ee2bb4e86ce";
}

} // namespace

const char* romGameName(RomGame game)
{
    switch (game) {
    case RomGame::SuperMario64: return "Super Mario 64";
    case RomGame::MarioKart64: return "Mario Kart 64";
    case RomGame::Unknown: return "Unknown";
    }
    return "Unknown";
}

RomImage classifyRom(std::filesystem::path path, std::vector<std::uint8_t> bytes)
{
    RomImage image;
    image.path = std::move(path);
    image.filename = image.path.filename().string();
    image.bytes = byteswappedToBigEndian(bytes);

    util::Sha1 sha1;
    sha1.update(image.bytes);
    image.sha1 = util::Sha1::toHex(sha1.final());
    image.internalName = headerName(image.bytes);

    const std::string name = uppercase(image.internalName);
    if (isSm64UsSha1(image.sha1) || name.find("SUPER MARIO 64") != std::string::npos) {
        image.game = RomGame::SuperMario64;
        image.supported = isSm64UsSha1(image.sha1);
    } else if (name.find("MARIOKART64") != std::string::npos || name.find("MARIO KART 64") != std::string::npos) {
        image.game = RomGame::MarioKart64;
        image.supported = true;
    }

    return image;
}

} // namespace sm64ps::assets
