#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

// Minimal structured logger: timestamped "LEVEL message key=value key=value" lines.
// INFO/WARN go to stdout, ERROR goes to stderr.
class Logger {
public:
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

private:
    static void log(const std::string& level, const std::string& message, bool toStderr);
};

#endif // LOGGER_HPP
