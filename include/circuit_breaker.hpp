#ifndef CIRCUIT_BREAKER_HPP
#define CIRCUIT_BREAKER_HPP

#include <chrono>
#include <string>
#include <unordered_map>

// Per-service circuit breaker: after `failureThreshold` consecutive
// failures, the circuit opens and rejects calls immediately (without
// hitting the network) for `cooldown`, giving a struggling upstream API
// time to recover instead of being hammered by retries.
class CircuitBreaker {
public:
    static CircuitBreaker& instance();

    // Throws runtime_error if the circuit for `service` is open.
    void checkAllowed(const std::string& service);
    void recordSuccess(const std::string& service);
    void recordFailure(const std::string& service);

private:
    struct State {
        int consecutiveFailures = 0;
        std::chrono::steady_clock::time_point openedAt{};
        bool open = false;
    };

    static constexpr int failureThreshold = 5;
    static constexpr std::chrono::seconds cooldown{30};

    std::unordered_map<std::string, State> states;
};

#endif // CIRCUIT_BREAKER_HPP
