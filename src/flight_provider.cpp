#include "flight_provider.hpp"
#include "flight_parser.hpp"
#include "logger.hpp"
#include "json.hpp"
#include <algorithm>
#include <ctime>
#include <functional>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace {

// Builds a tm for `date` (YYYY-MM-DD) at the given hour/minute.
tm makeTime(const std::string& date, int hour, int minute) {
    tm t = {};
    std::istringstream(date) >> std::get_time(&t, "%Y-%m-%d");
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = 0;
    return t;
}

// ---------------------------------------------------------------------------
// Amadeus: real GDS inventory. Preferred when credentials are available.
// ---------------------------------------------------------------------------
class AmadeusFlightProvider : public FlightProvider {
public:
    explicit AmadeusFlightProvider(FlightProviderConfig config) : config_(std::move(config)) {}

    std::string name() const override { return "amadeus"; }
    bool isLiveInventory() const override { return true; }

    std::vector<Flight> search(const std::string& from, const std::string& to,
                               const std::string& date, int passengers) override {
        std::string token = fetchToken();
        std::string fromIATA = config_.resolveIATA(from);
        std::string toIATA = config_.resolveIATA(to);

        json requestBody = {
            {"currencyCode", config_.currency},
            {"originDestinations", {{
                {"id", "1"},
                {"originLocationCode", fromIATA},
                {"destinationLocationCode", toIATA},
                {"departureDateTimeRange", {{"date", date}}}
            }}},
            {"travelers", json::array()},
            {"sources", {"GDS"}},
            {"searchCriteria", {
                {"maxFlightOffers", 10},
                {"flightFilters", {
                    {"cabinRestrictions", {{
                        {"cabin", "ECONOMY"},
                        {"coverage", "MOST_SEGMENTS"},
                        {"originDestinationIds", {"1"}}
                    }}}
                }}
            }}
        };

        for (int i = 1; i <= passengers; i++) {
            requestBody["travelers"].push_back({
                {"id", std::to_string(i)},
                {"travelerType", "ADULT"}
            });
        }

        std::string response = config_.httpRequest(config_.amadeusFlightUrl, "POST",
                                                   requestBody.dump(), token);
        return FlightParser::parseAmadeusFlightOffers(response, config_.currency);
    }

private:
    std::string fetchToken() {
        std::string payload = "grant_type=client_credentials&"
                              "client_id=" + config_.amadeusClientId + "&"
                              "client_secret=" + config_.amadeusClientSecret;
        std::string response = config_.httpRequest(config_.amadeusTokenUrl, "POST", payload, "");
        json j = json::parse(response);
        if (!j.contains("access_token")) {
            throw std::runtime_error("No access_token in Amadeus response");
        }
        return j["access_token"].get<std::string>();
    }

    FlightProviderConfig config_;
};

// ---------------------------------------------------------------------------
// Gemini estimator: no flight-inventory vendor required, only the Gemini key
// this project already uses for hotels and itineraries. Returns realistic
// representative options for a route - useful for planning and budgeting, but
// explicitly marked as estimates because they are not bookable inventory.
// ---------------------------------------------------------------------------
class GeminiFlightProvider : public FlightProvider {
public:
    explicit GeminiFlightProvider(FlightProviderConfig config) : config_(std::move(config)) {}

    std::string name() const override { return "gemini"; }
    bool isLiveInventory() const override { return false; }

    std::vector<Flight> search(const std::string& from, const std::string& to,
                               const std::string& date, int passengers) override {
        std::string prompt =
            "List 5 realistic economy flight options from " + from + " to " + to +
            " on " + date + " for " + std::to_string(passengers) + " passenger(s). "
            "Use real airlines and plausible schedules for this route. "
            "Give departure_time and arrival_time as 24-hour HH:MM local times, "
            "airport codes as 3-letter IATA codes, and price as a typical "
            "per-passenger fare in " + config_.currency + ".";

        json request = {
            {"contents", {{
                {"parts", {{{"text", prompt}}}}
            }}},
            {"generationConfig", {
                {"responseMimeType", "application/json"},
                {"responseSchema", {
                    {"type", "ARRAY"},
                    {"items", {
                        {"type", "OBJECT"},
                        {"properties", {
                            {"airline", {{"type", "STRING"}}},
                            {"flight_number", {{"type", "STRING"}}},
                            {"departure_airport", {{"type", "STRING"}}},
                            {"arrival_airport", {{"type", "STRING"}}},
                            {"departure_time", {{"type", "STRING"}}},
                            {"arrival_time", {{"type", "STRING"}}},
                            {"price", {{"type", "NUMBER"}}},
                            {"available_seats", {{"type", "INTEGER"}}}
                        }},
                        {"required", {"airline", "flight_number", "departure_airport",
                                      "arrival_airport", "departure_time", "arrival_time", "price"}}
                    }}
                }}
            }}
        };

        std::string url = config_.geminiApiUrl + "?key=" + config_.geminiApiKey;
        std::string response = config_.httpRequest(url, "POST", request.dump(), "");

        json responseJson = json::parse(response);
        if (!responseJson.contains("candidates") || responseJson["candidates"].empty()) {
            throw std::runtime_error("Invalid Gemini response structure");
        }

        std::string text = responseJson["candidates"][0]["content"]["parts"][0]["text"];
        return FlightParser::parseEstimatedFlights(text, date, config_.currency);
    }

private:
    FlightProviderConfig config_;
};

// ---------------------------------------------------------------------------
// Offline mock: no network, no credentials. Keeps the CLI, the REST API and
// the demo frontend fully exercisable (and CI deterministic) when no flight
// backend is configured at all.
// ---------------------------------------------------------------------------
class MockFlightProvider : public FlightProvider {
public:
    explicit MockFlightProvider(FlightProviderConfig config) : config_(std::move(config)) {}

    std::string name() const override { return "mock"; }
    bool isLiveInventory() const override { return false; }

    std::vector<Flight> search(const std::string& from, const std::string& to,
                               const std::string& date, int passengers) override {
        // Seed from the query so the same search always yields the same
        // results - stable demos and reproducible tests.
        std::seed_seq seed{std::hash<std::string>{}(from + "|" + to + "|" + date)};
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> priceDist(3000, 12000);
        std::uniform_int_distribution<int> seatDist(1, 9);
        std::uniform_int_distribution<int> durationDist(2, 5);

        const std::vector<std::pair<std::string, std::string>> carriers = {
            {"AI", "Air India"}, {"6E", "IndiGo"}, {"UK", "Vistara"},
            {"SG", "SpiceJet"}, {"QP", "Akasa Air"}
        };

        std::string fromCode = toCode(from);
        std::string toCode_ = toCode(to);

        std::vector<Flight> flights;
        for (size_t i = 0; i < carriers.size(); ++i) {
            int depHour = 6 + static_cast<int>(i) * 3;
            int duration = durationDist(rng);
            tm departure = makeTime(date, depHour, 25);
            tm arrival = makeTime(date, (depHour + duration) % 24, 15);

            flights.emplace_back(carriers[i].first,
                                 std::to_string(1000 + static_cast<int>(i) * 111),
                                 fromCode, toCode_, departure, arrival,
                                 static_cast<double>(priceDist(rng)),
                                 std::max(seatDist(rng), passengers),
                                 config_.currency, "mock");
        }

        std::sort(flights.begin(), flights.end(), [](const Flight& a, const Flight& b) {
            return a.getPrice() < b.getPrice();
        });
        return flights;
    }

private:
    // Best-effort 3-letter code from a city name, without a network call.
    static std::string toCode(const std::string& city) {
        std::string code;
        for (char c : city) {
            if (isalpha(static_cast<unsigned char>(c))) code += toupper(c);
            if (code.size() == 3) break;
        }
        while (code.size() < 3) code += 'X';
        return code;
    }

    FlightProviderConfig config_;
};

} // namespace

std::unique_ptr<FlightProvider> makeFlightProvider(const std::string& preference,
                                                   const FlightProviderConfig& config) {
    bool hasAmadeus = !config.amadeusClientId.empty() && !config.amadeusClientSecret.empty();
    bool hasGemini = !config.geminiApiKey.empty();

    std::string choice = preference.empty() ? "auto" : preference;

    if (choice == "auto") {
        if (hasAmadeus) choice = "amadeus";
        else if (hasGemini) choice = "gemini";
        else choice = "mock";
    }

    if (choice == "amadeus") {
        if (!hasAmadeus) {
            throw std::runtime_error("FLIGHT_PROVIDER=amadeus but AMADEUS_CLIENT_ID/"
                                     "AMADEUS_CLIENT_SECRET are not set");
        }
        Logger::info("Flight provider: amadeus (live inventory)");
        return std::make_unique<AmadeusFlightProvider>(config);
    }

    if (choice == "gemini") {
        if (!hasGemini) {
            throw std::runtime_error("FLIGHT_PROVIDER=gemini but GEMINI_API_KEY is not set");
        }
        Logger::warn("Flight provider: gemini - results are ESTIMATES, not bookable inventory");
        return std::make_unique<GeminiFlightProvider>(config);
    }

    if (choice == "mock") {
        Logger::warn("Flight provider: mock - synthetic offline data, not real flights");
        return std::make_unique<MockFlightProvider>(config);
    }

    throw std::runtime_error("Unknown FLIGHT_PROVIDER '" + choice +
                             "' (expected amadeus, gemini, mock or auto)");
}
