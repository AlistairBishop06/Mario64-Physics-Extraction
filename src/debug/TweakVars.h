#pragma once

#include "mario/MarioController.h"

#include <map>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace sm64ps::debug {

class TweakVars {
public:
    void setNumber(std::string name, double value);
    std::optional<double> number(const std::string& name) const;
    bool applyTo(mario::MovementTuning& tuning) const;

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);

    const std::map<std::string, double>& numbers() const { return numbers_; }

private:
    std::map<std::string, double> numbers_;
};

TweakVars makeDefaultMovementTweaks();

} // namespace sm64ps::debug

