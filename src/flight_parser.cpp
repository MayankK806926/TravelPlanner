#include "flight_parser.hpp"
#include "logger.hpp"
#include "json.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <stdexcept>

using json = nlohmann::json;

namespace FlightParser {

std::vector<Flight> parseAmadeusFlightOffers(const std::string& response,
                                             const std::string& currency) {
    std::vector<Flight> flights;
    try {
        json responseJson = json::parse(response);

        if (!responseJson.contains("data")) {
            if (responseJson.contains("errors")) {
                throw std::runtime_error(responseJson["errors"].dump(2));
            }
            return flights;
        }

        auto offers = responseJson["data"];
        std::sort(offers.begin(), offers.end(),
             [](const json& a, const json& b) {
                 return std::stod(a["price"]["total"].get<std::string>()) <
                        std::stod(b["price"]["total"].get<std::string>());
             });

        size_t numOffers = std::min(size_t(10), offers.size());
        for (size_t i = 0; i < numOffers; ++i) {
            try {
                const auto& offer = offers[i];
                const auto& itinerary = offer.at("itineraries").at(0);

                for (const auto& segment : itinerary.at("segments")) {
                    std::string airline = segment.at("carrierCode").get<std::string>();
                    std::string flightNumber = segment.at("number").get<std::string>();
                    std::string depAirport = segment.at("departure").at("iataCode").get<std::string>();
                    std::string arrAirport = segment.at("arrival").at("iataCode").get<std::string>();
                    std::string depTime = segment.at("departure").at("at").get<std::string>();
                    std::string arrTime = segment.at("arrival").at("at").get<std::string>();
                    double price = std::stod(offer.at("price").at("total").get<std::string>());
                    int availableSeats = offer.value("numberOfBookableSeats", 1);

                    tm departure = {}, arrival = {};
                    std::istringstream(depTime) >> std::get_time(&departure, "%Y-%m-%dT%H:%M:%S");
                    std::istringstream(arrTime) >> std::get_time(&arrival, "%Y-%m-%dT%H:%M:%S");

                    flights.emplace_back(airline, flightNumber, depAirport, arrAirport,
                                       departure, arrival, price, availableSeats, currency);
                }
            } catch (const std::exception& e) {
                Logger::warn(std::string("Skipping malformed flight offer: ") + e.what());
                continue;
            }
        }
    } catch (const json::exception& e) {
        throw std::runtime_error("Error parsing flight offers: " + std::string(e.what()));
    }

    return flights;
}

} // namespace FlightParser
