#ifndef FLIGHT_PARSER_HPP
#define FLIGHT_PARSER_HPP

#include <string>
#include <vector>
#include "flight.hpp"

// Pure parsing of an Amadeus flight-offers response payload. Deliberately
// free of any network/curl dependency so it can be unit tested directly
// against recorded API responses.
namespace FlightParser {

// Parses offers, sorted cheapest-first, skipping individual malformed
// offers rather than failing the whole batch. Throws only if the payload
// itself is not parseable JSON or reports an API-level error.
std::vector<Flight> parseAmadeusFlightOffers(const std::string& response,
                                             const std::string& currency = "INR");

} // namespace FlightParser

#endif // FLIGHT_PARSER_HPP
