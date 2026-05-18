#include "debug/Console.h"

#include <sstream>

namespace sm64ps::debug {

namespace {

std::vector<std::string> split(const std::string& line)
{
    std::istringstream stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

} // namespace

void Console::registerCommand(std::string name, std::string help, Handler handler)
{
    commands_[std::move(name)] = Command { std::move(help), std::move(handler) };
}

ConsoleResult Console::execute(const std::string& line) const
{
    const auto tokens = split(line);
    if (tokens.empty()) {
        return { false, "" };
    }

    const auto iter = commands_.find(tokens.front());
    if (iter == commands_.end()) {
        return { false, "unknown command: " + tokens.front() };
    }

    return iter->second.handler(tokens);
}

std::vector<std::string> Console::helpLines() const
{
    std::vector<std::string> lines;
    for (const auto& [name, command] : commands_) {
        lines.push_back(name + " - " + command.help);
    }
    return lines;
}

} // namespace sm64ps::debug

