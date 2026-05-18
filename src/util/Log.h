#pragma once

#include <mutex>
#include <sstream>
#include <string>
#include <utility>

namespace sm64ps::util {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error
};

class Logger {
public:
    static Logger& instance();

    void setLevel(LogLevel level);
    LogLevel level() const;

    template <typename... Args>
    void write(LogLevel level, Args&&... args)
    {
        if (level < level_) {
            return;
        }

        std::ostringstream stream;
        (stream << ... << args);
        writeLine(level, stream.str());
    }

private:
    Logger() = default;

    void writeLine(LogLevel level, const std::string& message);

    LogLevel level_ = LogLevel::Info;
    mutable std::mutex mutex_;
};

template <typename... Args>
void logTrace(Args&&... args) { Logger::instance().write(LogLevel::Trace, std::forward<Args>(args)...); }

template <typename... Args>
void logDebug(Args&&... args) { Logger::instance().write(LogLevel::Debug, std::forward<Args>(args)...); }

template <typename... Args>
void logInfo(Args&&... args) { Logger::instance().write(LogLevel::Info, std::forward<Args>(args)...); }

template <typename... Args>
void logWarn(Args&&... args) { Logger::instance().write(LogLevel::Warn, std::forward<Args>(args)...); }

template <typename... Args>
void logError(Args&&... args) { Logger::instance().write(LogLevel::Error, std::forward<Args>(args)...); }

} // namespace sm64ps::util
