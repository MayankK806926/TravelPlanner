#include <catch2/catch_test_macros.hpp>
#include "retry.hpp"
#include <stdexcept>

TEST_CASE("returns result immediately on success", "[retry]") {
    int calls = 0;
    int result = retryWithBackoff<int>("test", [&]() -> int {
        calls++;
        return 42;
    });
    CHECK(result == 42);
    CHECK(calls == 1);
}

TEST_CASE("retries on HTTP 503 and eventually succeeds", "[retry]") {
    int calls = 0;
    int result = retryWithBackoff<int>("test", [&]() -> int {
        calls++;
        if (calls < 3) throw std::runtime_error("HTTP error 503: overloaded");
        return 7;
    }, /*maxRetries=*/3);
    CHECK(result == 7);
    CHECK(calls == 3);
}

TEST_CASE("does not retry non-503 errors", "[retry]") {
    int calls = 0;
    REQUIRE_THROWS_AS(
        retryWithBackoff<int>("test", [&]() -> int {
            calls++;
            throw std::runtime_error("HTTP error 400: bad request");
        }),
        std::runtime_error
    );
    CHECK(calls == 1);
}

TEST_CASE("gives up after maxRetries and throws", "[retry]") {
    int calls = 0;
    REQUIRE_THROWS_AS(
        retryWithBackoff<int>("test", [&]() -> int {
            calls++;
            throw std::runtime_error("HTTP error 503: overloaded");
        }, /*maxRetries=*/2),
        std::runtime_error
    );
    CHECK(calls == 2);
}
