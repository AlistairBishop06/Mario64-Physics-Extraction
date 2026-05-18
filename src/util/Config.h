#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace sm64ps::util {

class Config {
public:
    bool load(const std::filesystem::path& path);
    bool save(const std::filesystem::path& path) const;

    template <typename T>
    T value(const std::string& pointer, const T& fallback) const
    {
        const nlohmann::json::json_pointer jsonPointer(pointer);
        if (!data_.contains(jsonPointer)) {
            return fallback;
        }
        return data_.at(jsonPointer).get<T>();
    }

    template <typename T>
    void set(const std::string& pointer, const T& value)
    {
        data_[nlohmann::json::json_pointer(pointer)] = value;
    }

    const nlohmann::json& data() const { return data_; }
    nlohmann::json& data() { return data_; }

private:
    nlohmann::json data_ = nlohmann::json::object();
};

} // namespace sm64ps::util

