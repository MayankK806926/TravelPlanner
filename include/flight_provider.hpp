#ifndef FLIGHT_PROVIDER_HPP
#define FLIGHT_PROVIDER_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "flight.hpp"

// Flight inventory is the one part of this application with no stable free
// data source: the Amadeus self-service tier is not always available to a
// given developer. Rather than hard-wiring one vendor, flight search sits
// behind this interface so a backend can be swapped by configuration alone.
class FlightProvider {
public:
    virtual ~FlightProvider() = default;

    virtual std::vector<Flight> search(const std::string& from,
                                       const std::string& to,
                                       const std::string& date,
                                       int passengers) = 0;

    // Short identifier, also used as the circuit-breaker service key.
    virtual std::string name() const = 0;

    // True when results are real bookable inventory rather than estimates.
    virtual bool isLiveInventory() const = 0;
};

// Everything a provider needs from the outside world, injected rather than
// reached for globally so providers stay unit-testable with fakes.
struct FlightProviderConfig {
    std::function<std::string(const std::string& url,
                              const std::string& method,
                              const std::string& data,
                              const std::string& token)> httpRequest;
    std::function<std::string(const std::string& city)> resolveIATA;

    std::string currency = "INR";

    std::string amadeusClientId;
    std::string amadeusClientSecret;
    std::string amadeusTokenUrl;
    std::string amadeusFlightUrl;

    std::string geminiApiKey;
    std::string geminiApiUrl;
};

// Chooses a backend. `preference` accepts "amadeus", "gemini", "mock" or
// "auto"; "auto" picks Amadeus when its credentials are present, else the
// Gemini estimator when that key is present, else the offline mock.
std::unique_ptr<FlightProvider> makeFlightProvider(const std::string& preference,
                                                   const FlightProviderConfig& config);

#endif // FLIGHT_PROVIDER_HPP
