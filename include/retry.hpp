#ifndef RETRY_HPP
#define RETRY_HPP

#include <chrono>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include "logger.hpp"

// Retries `fn` up to maxRetries times with exponential backoff whenever it
// throws an exception whose message contains "HTTP error 503" (service
// overloaded). Any other exception propagates immediately. After the last
// attempt fails, the last exception is rethrown wrapped with `errorPrefix`.
template <typename T>
T retryWithBackoff(const std::string& errorPrefix, std::function<T()> fn, int maxRetries = 3) {
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        try {
            return fn();
        } catch (const std::exception& e) {
            std::string msg = e.what();
            bool retryable = msg.find("HTTP error 503") != std::string::npos;
            if (retryable && attempt < maxRetries - 1) {
                int delaySeconds = 2 * (attempt + 1);
                Logger::warn(errorPrefix + ": service overloaded (503), retrying in " +
                             std::to_string(delaySeconds) + "s (attempt " +
                             std::to_string(attempt + 1) + "/" + std::to_string(maxRetries) + ")");
                std::this_thread::sleep_for(std::chrono::seconds(delaySeconds));
                continue;
            }
            throw std::runtime_error(errorPrefix + ": " + msg);
        }
    }
    throw std::runtime_error(errorPrefix + ": exhausted retries");
}

#endif // RETRY_HPP
