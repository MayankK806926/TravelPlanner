#include "circuit_breaker.hpp"
#include "logger.hpp"
#include <stdexcept>

CircuitBreaker& CircuitBreaker::instance() {
    static CircuitBreaker breaker;
    return breaker;
}

void CircuitBreaker::checkAllowed(const std::string& service) {
    auto& state = states[service];
    if (!state.open) return;

    auto elapsed = std::chrono::steady_clock::now() - state.openedAt;
    if (elapsed >= cooldown) {
        // Half-open: let one request through to probe recovery.
        state.open = false;
        state.consecutiveFailures = 0;
        return;
    }

    throw std::runtime_error("Circuit open for " + service + ": too many recent failures, backing off");
}

void CircuitBreaker::recordSuccess(const std::string& service) {
    auto& state = states[service];
    state.consecutiveFailures = 0;
    state.open = false;
}

void CircuitBreaker::recordFailure(const std::string& service) {
    auto& state = states[service];
    state.consecutiveFailures++;
    if (state.consecutiveFailures >= failureThreshold && !state.open) {
        state.open = true;
        state.openedAt = std::chrono::steady_clock::now();
        Logger::error("Circuit breaker OPEN for " + service + " after " +
                      std::to_string(state.consecutiveFailures) + " consecutive failures");
    }
}
