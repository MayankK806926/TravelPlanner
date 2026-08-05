#include "logger.hpp"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {
std::string timestamp() {
    time_t now = time(nullptr);
    tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream oss;
    oss << std::put_time(&local, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}
}

void Logger::log(const std::string& level, const std::string& message, bool toStderr) {
    std::ostream& out = toStderr ? std::cerr : std::cout;
    out << "[" << timestamp() << "] [" << level << "] " << message << "\n";
}

void Logger::info(const std::string& message) { log("INFO", message, false); }
void Logger::warn(const std::string& message) { log("WARN", message, true); }
void Logger::error(const std::string& message) { log("ERROR", message, true); }
