#include <catch2/catch_test_macros.hpp>
#include "flight_provider.hpp"
#include "flight_parser.hpp"
#include <stdexcept>

namespace {

// A config with no real network: httpRequest returns a canned payload so
// providers can be exercised without touching the internet.
FlightProviderConfig configWith(const std::string& geminiKey,
                                const std::string& amadeusId,
                                const std::string& cannedResponse = "") {
    FlightProviderConfig config;
    config.geminiApiKey = geminiKey;
    config.geminiApiUrl = "https://example.invalid/gemini";
    config.amadeusClientId = amadeusId;
    config.amadeusClientSecret = amadeusId.empty() ? "" : "secret";
    config.currency = "INR";
    config.resolveIATA = [](const std::string&) { return "XXX"; };
    config.httpRequest = [cannedResponse](const std::string&, const std::string&,
                                          const std::string&, const std::string&) {
        return cannedResponse;
    };
    return config;
}

} // namespace

TEST_CASE("auto selection prefers amadeus when credentials exist", "[flight_provider]") {
    auto provider = makeFlightProvider("auto", configWith("gemini-key", "amadeus-id"));
    CHECK(provider->name() == "amadeus");
    CHECK(provider->isLiveInventory());
}

TEST_CASE("auto falls back to gemini when amadeus is unavailable", "[flight_provider]") {
    auto provider = makeFlightProvider("auto", configWith("gemini-key", ""));
    CHECK(provider->name() == "gemini");
    CHECK_FALSE(provider->isLiveInventory());
}

TEST_CASE("auto falls back to mock when nothing is configured", "[flight_provider]") {
    auto provider = makeFlightProvider("auto", configWith("", ""));
    CHECK(provider->name() == "mock");
    CHECK_FALSE(provider->isLiveInventory());
}

TEST_CASE("explicitly requesting an unconfigured provider fails loudly", "[flight_provider]") {
    CHECK_THROWS_AS(makeFlightProvider("amadeus", configWith("gemini-key", "")), std::runtime_error);
    CHECK_THROWS_AS(makeFlightProvider("gemini", configWith("", "amadeus-id")), std::runtime_error);
    CHECK_THROWS_AS(makeFlightProvider("nonsense", configWith("", "")), std::runtime_error);
}

TEST_CASE("mock provider returns deterministic, sorted, non-bookable results", "[flight_provider]") {
    auto provider = makeFlightProvider("mock", configWith("", ""));

    auto first = provider->search("Delhi", "Bangalore", "2026-09-10", 2);
    auto second = provider->search("Delhi", "Bangalore", "2026-09-10", 2);

    REQUIRE_FALSE(first.empty());
    REQUIRE(first.size() == second.size());

    // Same query -> same results, so demos and tests are reproducible.
    for (size_t i = 0; i < first.size(); ++i) {
        CHECK(first[i].getPrice() == second[i].getPrice());
        CHECK(first[i].getFlightNumber() == second[i].getFlightNumber());
    }

    // Cheapest first.
    for (size_t i = 1; i < first.size(); ++i) {
        CHECK(first[i - 1].getPrice() <= first[i].getPrice());
    }

    // Never claims to be bookable inventory.
    for (const auto& f : first) {
        CHECK(f.getSource() == "mock");
        CHECK_FALSE(f.isBookable());
        CHECK(f.getAvailableSeats() >= 2);
    }
}

TEST_CASE("mock provider varies by route", "[flight_provider]") {
    auto provider = makeFlightProvider("mock", configWith("", ""));
    auto delToBlr = provider->search("Delhi", "Bangalore", "2026-09-10", 1);
    auto bomToGoi = provider->search("Mumbai", "Goa", "2026-09-10", 1);

    REQUIRE_FALSE(delToBlr.empty());
    REQUIRE_FALSE(bomToGoi.empty());
    CHECK(delToBlr[0].getDepartureAirport() != bomToGoi[0].getDepartureAirport());
}

TEST_CASE("estimated flights parse and are marked as estimates", "[flight_provider]") {
    std::string payload = R"([
        {"airline":"6E","flight_number":"6E-2011","departure_airport":"DEL",
         "arrival_airport":"BLR","departure_time":"06:30","arrival_time":"09:15",
         "price":7400.0,"available_seats":5},
        {"airline":"AI","flight_number":"AI-803","departure_airport":"DEL",
         "arrival_airport":"BLR","departure_time":"11:00","arrival_time":"13:50",
         "price":5200.0,"available_seats":3}
    ])";

    auto flights = FlightParser::parseEstimatedFlights(payload, "2026-09-10", "INR");

    REQUIRE(flights.size() == 2);
    // Sorted cheapest-first.
    CHECK(flights[0].getPrice() == 5200.0);
    CHECK(flights[0].getFlightNumber() == "AI-803");
    for (const auto& f : flights) {
        CHECK(f.getSource() == "estimate");
        CHECK_FALSE(f.isBookable());
    }
}

TEST_CASE("estimated flight parsing skips malformed entries", "[flight_provider]") {
    std::string payload = R"([
        {"airline":"6E"},
        {"airline":"AI","flight_number":"AI-803","departure_airport":"DEL",
         "arrival_airport":"BLR","departure_time":"11:00","arrival_time":"13:50",
         "price":5200.0}
    ])";

    auto flights = FlightParser::parseEstimatedFlights(payload, "2026-09-10", "INR");
    REQUIRE(flights.size() == 1);
    CHECK(flights[0].getFlightNumber() == "AI-803");
}

TEST_CASE("estimated flight parsing rejects non-array payloads", "[flight_provider]") {
    CHECK_THROWS_AS(FlightParser::parseEstimatedFlights("{}", "2026-09-10"), std::runtime_error);
    CHECK_THROWS_AS(FlightParser::parseEstimatedFlights("not json", "2026-09-10"), std::runtime_error);
}
