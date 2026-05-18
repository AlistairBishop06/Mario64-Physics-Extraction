#include "debug/TweakVars.h"

namespace sm64ps::debug {

void TweakVars::setNumber(std::string name, double value)
{
    numbers_[std::move(name)] = value;
}

std::optional<double> TweakVars::number(const std::string& name) const
{
    const auto iter = numbers_.find(name);
    if (iter == numbers_.end()) {
        return std::nullopt;
    }
    return iter->second;
}

bool TweakVars::applyTo(mario::MovementTuning& tuning) const
{
    auto apply = [&](const char* name, float& target) {
        if (auto value = number(name)) {
            target = static_cast<float>(*value);
            return true;
        }
        return false;
    };

    bool changed = false;
    changed |= apply("walk_speed", tuning.walkSpeed);
    changed |= apply("run_speed", tuning.runSpeed);
    changed |= apply("ground_accel", tuning.groundAccel);
    changed |= apply("ground_decel", tuning.groundDecel);
    changed |= apply("air_accel", tuning.airAccel);
    changed |= apply("jump_velocity", tuning.jumpVelocity);
    changed |= apply("gravity", tuning.gravity);
    changed |= apply("terminal_velocity", tuning.terminalVelocity);
    changed |= apply("long_jump_forward_speed", tuning.longJumpForwardSpeed);
    changed |= apply("dive_forward_speed", tuning.diveForwardSpeed);
    changed |= apply("wall_kick_speed", tuning.wallKickSpeed);
    return changed;
}

nlohmann::json TweakVars::toJson() const
{
    return numbers_;
}

void TweakVars::fromJson(const nlohmann::json& json)
{
    numbers_.clear();
    for (auto iter = json.begin(); iter != json.end(); ++iter) {
        if (iter.value().is_number()) {
            numbers_[iter.key()] = iter.value().get<double>();
        }
    }
}

TweakVars makeDefaultMovementTweaks()
{
    mario::MovementTuning defaults;
    TweakVars tweaks;
    tweaks.setNumber("walk_speed", defaults.walkSpeed);
    tweaks.setNumber("run_speed", defaults.runSpeed);
    tweaks.setNumber("ground_accel", defaults.groundAccel);
    tweaks.setNumber("ground_decel", defaults.groundDecel);
    tweaks.setNumber("air_accel", defaults.airAccel);
    tweaks.setNumber("jump_velocity", defaults.jumpVelocity);
    tweaks.setNumber("gravity", defaults.gravity);
    tweaks.setNumber("terminal_velocity", defaults.terminalVelocity);
    tweaks.setNumber("long_jump_forward_speed", defaults.longJumpForwardSpeed);
    tweaks.setNumber("dive_forward_speed", defaults.diveForwardSpeed);
    tweaks.setNumber("wall_kick_speed", defaults.wallKickSpeed);
    return tweaks;
}

} // namespace sm64ps::debug

