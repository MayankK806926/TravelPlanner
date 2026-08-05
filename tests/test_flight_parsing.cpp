#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include "flight_parser.hpp"

TEST_CASE("parses a well-formed Amadeus flight-offers response", "[flight_parsing]") {
    std::string response = R"({
        "data": [
            {
                "price": {"total": "3085.00"},
                "numberOfBookableSeats": 9,
                "itineraries": [{
                    "segments": [{
                        "carrierCode": "AI",
                        "number": "2803",
                        "departure": {"iataCode": "DEL", "at": "2026-09-10T06:25:00"},
                        "arrival": {"iataCode": "BLR", "at": "2026-09-10T09:25:00"}
                    }]
                }]
            }
        ]
    })";

    auto flights = FlightParser::parseAmadeusFlightOffers(response);

    REQUIRE(flights.size() == 1);
    CHECK(flights[0].getAirline() == "AI");
    CHECK(flights[0].getFlightNumber() == "2803");
    CHECK(flights[0].getDepartureAirport() == "DEL");
    CHECK(flights[0].getArrivalAirport() == "BLR");
    CHECK(flights[0].getPrice() == 3085.00);
    CHECK(flights[0].getAvailableSeats() == 9);
}

TEST_CASE("sorts offers by price ascending", "[flight_parsing]") {
    std::string response = R"({
        "data": [
            {
                "price": {"total": "9000.00"},
                "itineraries": [{"segments": [{
                    "carrierCode": "6E", "number": "100",
                    "departure": {"iataCode": "DEL", "at": "2026-09-10T06:00:00"},
                    "arrival": {"iataCode": "BLR", "at": "2026-09-10T09:00:00"}
                }]}]
            },
            {
                "price": {"total": "3000.00"},
                "itineraries": [{"segments": [{
                    "carrierCode": "AI", "number": "200",
                    "departure": {"iataCode": "DEL", "at": "2026-09-10T07:00:00"},
                    "arrival": {"iataCode": "BLR", "at": "2026-09-10T10:00:00"}
                }]}]
            }
        ]
    })";

    auto flights = FlightParser::parseAmadeusFlightOffers(response);

    REQUIRE(flights.size() == 2);
    CHECK(flights[0].getPrice() == 3000.00);
    CHECK(flights[1].getPrice() == 9000.00);
}

TEST_CASE("surfaces API-level errors as an exception", "[flight_parsing]") {
    std::string response = R"({"errors": [{"detail": "no offers found"}]})";
    REQUIRE_THROWS_AS(FlightParser::parseAmadeusFlightOffers(response), std::runtime_error);
}

TEST_CASE("returns empty vector when response has neither data nor errors", "[flight_parsing]") {
    std::string response = R"({"meta": {"count": 0}})";
    auto flights = FlightParser::parseAmadeusFlightOffers(response);
    CHECK(flights.empty());
}

TEST_CASE("skips a malformed offer without failing the whole batch", "[flight_parsing]") {
    std::string response = R"({
        "data": [
            {"price": {"total": "1000.00"}},
            {
                "price": {"total": "5000.00"},
                "itineraries": [{"segments": [{
                    "carrierCode": "AI", "number": "300",
                    "departure": {"iataCode": "DEL", "at": "2026-09-10T06:00:00"},
                    "arrival": {"iataCode": "BLR", "at": "2026-09-10T09:00:00"}
                }]}]
            }
        ]
    })";

    auto flights = FlightParser::parseAmadeusFlightOffers(response);
    REQUIRE(flights.size() == 1);
    CHECK(flights[0].getFlightNumber() == "300");
}
