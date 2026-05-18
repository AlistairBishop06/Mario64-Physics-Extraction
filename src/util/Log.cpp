#include "util/Log.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace sm64ps::util {

namespace {

const char* toString(LogLevel level)
{
    switch (level) {
    case LogLevel::Trace: return "trace";
    case LogLevel::Debug: return "debug";
    case LogLevel::Info: return "info";
    case LogLevel::Warn: return "warn";
    case LogLevel::Error: return "error";
    }
    return "unknown";
}

} // namespace

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::level() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void Logger::writeLine(LogLevel level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);

    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime {};
#if defined(_WIN32)
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::ostream& out = level >= LogLevel::Warn ? std::cerr : std::cout;
    out << std::put_time(&localTime, "%H:%M:%S")
        << " [" << toString(level) << "] "
        << message << '\n';
}

} // namespace sm64ps::util

