#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace sm64ps::debug {

struct ConsoleResult {
    bool handled = false;
    std::string message;
};

class Console {
public:
    using Handler = std::function<ConsoleResult(const std::vector<std::string>&)>;

    void registerCommand(std::string name, std::string help, Handler handler);
    ConsoleResult execute(const std::string& line) const;
    std::vector<std::string> helpLines() const;

private:
    struct Command {
        std::string help;
        Handler handler;
    };

    std::map<std::string, Command> commands_;
};

} // namespace sm64ps::debug

