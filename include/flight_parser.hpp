#ifndef FLIGHT_PARSER_HPP
#define FLIGHT_PARSER_HPP

#include <string>
#include <vector>
#include "flight.hpp"

// Pure parsing of flight payloads. Deliberately free of any network/curl
// dependency so it can be unit tested directly against recorded responses.
namespace FlightParser {

// Parses offers, sorted cheapest-first, skipping individual malformed
// offers rather than failing the whole batch. Throws only if the payload
// itself is not parseable JSON or reports an API-level error.
std::vector<Flight> parseAmadeusFlightOffers(const std::string& response,
                                             const std::string& currency = "INR");

// Parses the schema-constrained JSON array produced by the Gemini flight
// estimator. Results are marked with source "estimate" - they are
// representative options, not bookable inventory.
std::vector<Flight> parseEstimatedFlights(const std::string& json,
                                          const std::string& date,
                                          const std::string& currency = "INR");

} // namespace FlightParser

#endif // FLIGHT_PARSER_HPP
