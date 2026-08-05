#include <catch2/catch_test_macros.hpp>
#include "date_utils.hpp"

TEST_CASE("valid dates are accepted", "[date_utils]") {
    REQUIRE(DateUtils::isValidDateString("2026-09-10"));
    REQUIRE(DateUtils::isValidDateString("2028-02-29")); // leap year
    REQUIRE(DateUtils::isValidDateString("2024-01-01"));
}

TEST_CASE("invalid calendar dates are rejected", "[date_utils]") {
    REQUIRE_FALSE(DateUtils::isValidDateString("2026-02-31")); // no such day
    REQUIRE_FALSE(DateUtils::isValidDateString("2026-13-01")); // no such month
    REQUIRE_FALSE(DateUtils::isValidDateString("2026-00-10")); // no month 0
    REQUIRE_FALSE(DateUtils::isValidDateString("2027-02-29")); // not a leap year
    REQUIRE_FALSE(DateUtils::isValidDateString("2100-02-29")); // century, not leap
}

TEST_CASE("malformed date strings are rejected", "[date_utils]") {
    REQUIRE_FALSE(DateUtils::isValidDateString("2026/09/10"));
    REQUIRE_FALSE(DateUtils::isValidDateString("10-09-2026"));
    REQUIRE_FALSE(DateUtils::isValidDateString("not-a-date"));
    REQUIRE_FALSE(DateUtils::isValidDateString(""));
    REQUIRE_FALSE(DateUtils::isValidDateString("2026-9-10")); // must be zero-padded
}

TEST_CASE("dates before minYear are rejected", "[date_utils]") {
    REQUIRE_FALSE(DateUtils::isValidDateString("2020-01-01"));
    REQUIRE(DateUtils::isValidDateString("2020-01-01", 2020));
}
