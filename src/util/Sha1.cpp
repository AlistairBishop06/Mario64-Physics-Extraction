#include "util/Sha1.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace sm64ps::util {

namespace {

std::uint32_t rotl(std::uint32_t value, std::uint32_t bits)
{
    return (value << bits) | (value >> (32U - bits));
}

std::uint32_t readBe32(const std::uint8_t* data)
{
    return (static_cast<std::uint32_t>(data[0]) << 24U)
        | (static_cast<std::uint32_t>(data[1]) << 16U)
        | (static_cast<std::uint32_t>(data[2]) << 8U)
        | static_cast<std::uint32_t>(data[3]);
}

void writeBe64(std::uint8_t* data, std::uint64_t value)
{
    for (int i = 7; i >= 0; --i) {
        data[i] = static_cast<std::uint8_t>(value & 0xffU);
        value >>= 8U;
    }
}

} // namespace

Sha1::Sha1()
    : state_ { 0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U, 0xc3d2e1f0U }
{
}

void Sha1::update(const std::uint8_t* data, std::size_t length)
{
    if (finalized_ || length == 0) {
        return;
    }

    totalBytes_ += length;

    std::size_t offset = 0;
    if (bufferSize_ > 0) {
        const std::size_t toCopy = std::min<std::size_t>(64 - bufferSize_, length);
        std::copy(data, data + toCopy, buffer_.begin() + static_cast<std::ptrdiff_t>(bufferSize_));
        bufferSize_ += toCopy;
        offset += toCopy;

        if (bufferSize_ == 64) {
            processBlock(buffer_.data());
            bufferSize_ = 0;
        }
    }

    while (offset + 64 <= length) {
        processBlock(data + offset);
        offset += 64;
    }

    if (offset < length) {
        bufferSize_ = length - offset;
        std::copy(data + offset, data + length, buffer_.begin());
    }
}

void Sha1::update(const std::vector<std::uint8_t>& data)
{
    update(data.data(), data.size());
}

std::array<std::uint8_t, 20> Sha1::final()
{
    if (!finalized_) {
        const std::uint64_t bitLength = totalBytes_ * 8U;
        std::uint8_t pad = 0x80U;
        update(&pad, 1);

        std::uint8_t zero = 0;
        while (bufferSize_ != 56) {
            update(&zero, 1);
        }

        std::array<std::uint8_t, 8> lengthBytes {};
        writeBe64(lengthBytes.data(), bitLength);
        update(lengthBytes.data(), lengthBytes.size());
        finalized_ = true;
    }

    std::array<std::uint8_t, 20> digest {};
    for (std::size_t i = 0; i < state_.size(); ++i) {
        digest[i * 4 + 0] = static_cast<std::uint8_t>((state_[i] >> 24U) & 0xffU);
        digest[i * 4 + 1] = static_cast<std::uint8_t>((state_[i] >> 16U) & 0xffU);
        digest[i * 4 + 2] = static_cast<std::uint8_t>((state_[i] >> 8U) & 0xffU);
        digest[i * 4 + 3] = static_cast<std::uint8_t>(state_[i] & 0xffU);
    }
    return digest;
}

std::optional<std::string> Sha1::fileHexDigest(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    Sha1 sha1;
    std::array<char, 8192> buffer {};
    while (file) {
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto read = file.gcount();
        if (read > 0) {
            sha1.update(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::size_t>(read));
        }
    }

    return toHex(sha1.final());
}

std::string Sha1::toHex(const std::array<std::uint8_t, 20>& digest)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (const auto byte : digest) {
        stream << std::setw(2) << static_cast<int>(byte);
    }
    return stream.str();
}

void Sha1::processBlock(const std::uint8_t* block)
{
    std::array<std::uint32_t, 80> words {};
    for (std::size_t i = 0; i < 16; ++i) {
        words[i] = readBe32(block + i * 4);
    }
    for (std::size_t i = 16; i < 80; ++i) {
        words[i] = rotl(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
    }

    std::uint32_t a = state_[0];
    std::uint32_t b = state_[1];
    std::uint32_t c = state_[2];
    std::uint32_t d = state_[3];
    std::uint32_t e = state_[4];

    for (std::size_t i = 0; i < 80; ++i) {
        std::uint32_t f = 0;
        std::uint32_t k = 0;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5a827999U;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ed9eba1U;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdcU;
        } else {
            f = b ^ c ^ d;
            k = 0xca62c1d6U;
        }

        const std::uint32_t temp = rotl(a, 5) + f + e + k + words[i];
        e = d;
        d = c;
        c = rotl(b, 30);
        b = a;
        a = temp;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
}

} // namespace sm64ps::util
