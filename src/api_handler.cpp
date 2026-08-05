#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "api_handler.hpp"
#include "flight_parser.hpp"
#include "retry.hpp"
#include "circuit_breaker.hpp"
#include "logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <curl/curl.h>
#include <iomanip>

using namespace std;
using json = nlohmann::json;

// Define static members
string APIHandler::GEMINI_API_KEY;
string APIHandler::GEMINI_API_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent";
string APIHandler::AMADEUS_CLIENT_ID;
string APIHandler::AMADEUS_CLIENT_SECRET;
string APIHandler::AMADEUS_TOKEN_URL = "https://test.api.amadeus.com/v1/security/oauth2/token";
string APIHandler::AMADEUS_FLIGHT_URL = "https://test.api.amadeus.com/v2/shopping/flight-offers";
string APIHandler::WEATHER_API_KEY;
string APIHandler::WEATHER_API_URL = "http://api.weatherapi.com/v1";
string APIHandler::CURRENCY_CODE = "INR";

namespace {

// Reads an env var, returns empty string if unset.
string getEnvOrEmpty(const char* name) {
    const char* value = getenv(name);
    return value ? string(value) : string();
}

// In-memory caches: IATA codes rarely change, weather is cheap to cache
// for a short TTL to avoid re-fetching within the same session.
unordered_map<string, string> iataCache;
struct WeatherCacheEntry { chrono::steady_clock::time_point fetchedAt; json data; };
unordered_map<string, WeatherCacheEntry> weatherCache;
const chrono::minutes weatherCacheTTL{30};

string weatherCacheKey(const string& city, int days) {
    return city + "|" + to_string(days);
}

} // namespace

// Initialize API keys: environment variables take priority; falls back to
// config/api_keys.json for local development if the env vars aren't set.
// Keys should never be committed to source control - see .gitignore.
void APIHandler::initializeAPIKeys() {
    GEMINI_API_KEY = getEnvOrEmpty("GEMINI_API_KEY");
    AMADEUS_CLIENT_ID = getEnvOrEmpty("AMADEUS_CLIENT_ID");
    AMADEUS_CLIENT_SECRET = getEnvOrEmpty("AMADEUS_CLIENT_SECRET");
    WEATHER_API_KEY = getEnvOrEmpty("WEATHER_API_KEY");
    string envCurrency = getEnvOrEmpty("CURRENCY_CODE");
    if (!envCurrency.empty()) CURRENCY_CODE = envCurrency;

    if (!GEMINI_API_KEY.empty() && !AMADEUS_CLIENT_ID.empty() &&
        !AMADEUS_CLIENT_SECRET.empty() && !WEATHER_API_KEY.empty()) {
        Logger::info("API keys loaded from environment variables");
        return;
    }

    try {
        ifstream config_file("config/api_keys.json");
        if (!config_file.is_open()) {
            throw runtime_error("No environment variables set and could not open config/api_keys.json");
        }

        json config = json::parse(config_file);

        if (GEMINI_API_KEY.empty()) GEMINI_API_KEY = config.value("gemini", json::object()).value("api_key", "");
        if (AMADEUS_CLIENT_ID.empty()) AMADEUS_CLIENT_ID = config.value("amadeus", json::object()).value("client_id", "");
        if (AMADEUS_CLIENT_SECRET.empty()) AMADEUS_CLIENT_SECRET = config.value("amadeus", json::object()).value("client_secret", "");
        if (WEATHER_API_KEY.empty()) WEATHER_API_KEY = config.value("weather", json::object()).value("api_key", "");

        Logger::info("API keys loaded from config/api_keys.json (fallback)");
    } catch (const exception& e) {
        throw runtime_error("Error loading API keys: " + string(e.what()));
    }
}

void APIHandler::clearCaches() {
    iataCache.clear();
    weatherCache.clear();
}

// Callback function to write response data
size_t APIHandler::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Make HTTP request
string APIHandler::makeHttpRequest(const string& url, const string& method, const string& data, const string& token) {
    CURL* curl = curl_easy_init();
    string response;

    if (curl) {
        try {
            struct curl_slist* headers = NULL;

            if (url.find("amadeus") != string::npos && url.find("oauth2/token") != string::npos) {
                headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
            } else {
                headers = curl_slist_append(headers, "Content-Type: application/json");
            }

            if (!token.empty()) {
                headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

            if (method == "POST") {
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                if (!data.empty()) {
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
                }
            }

            CURLcode res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                string error = "Curl failed: " + string(curl_easy_strerror(res));
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                throw runtime_error(error);
            }

            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            if (http_code >= 400) {
                string error = "HTTP error " + to_string(http_code) + ": " + response;
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                throw runtime_error(error);
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            return response;
        } catch (const exception& e) {
            curl_easy_cleanup(curl);
            throw;
        }
    }

    throw runtime_error("Failed to initialize CURL");
}

// Get weather forecast (console display)
void APIHandler::getWeather(const string& city, int days) {
    json result = getWeatherJson(city, days);
    cout << "\nWeather forecast for " << city << ":\n";
    for (const auto& day : result["forecast"]) {
        cout << "Date: " << day["date"].get<string>() << "\n"
             << "Max temp: " << day["max_temp_c"].get<double>() << " C\n"
             << "Min temp: " << day["min_temp_c"].get<double>() << " C\n"
             << "Condition: " << day["condition"].get<string>() << "\n"
             << "Rain chance: " << day["rain_chance"].get<int>() << "%\n\n";
    }
}

// Get weather forecast as JSON, backed by a short-lived cache.
json APIHandler::getWeatherJson(const string& city, int days) {
    string key = weatherCacheKey(city, days);
    auto it = weatherCache.find(key);
    if (it != weatherCache.end()) {
        auto age = chrono::steady_clock::now() - it->second.fetchedAt;
        if (age < weatherCacheTTL) {
            Logger::info("Weather cache hit for " + key);
            return it->second.data;
        }
        weatherCache.erase(it);
    }

    const string service = "weather";
    CircuitBreaker::instance().checkAllowed(service);

    try {
        string url = WEATHER_API_URL + "/forecast.json?key=" + WEATHER_API_KEY +
                    "&q=" + urlEncode(city) + "&days=" + to_string(days);
        string response = makeHttpRequest(url);
        json responseJson = json::parse(response);

        json result;
        result["city"] = city;
        result["forecast"] = json::array();
        for (const auto& day : responseJson["forecast"]["forecastday"]) {
            result["forecast"].push_back({
                {"date", day["date"].get<string>()},
                {"max_temp_c", day["day"]["maxtemp_c"].get<double>()},
                {"min_temp_c", day["day"]["mintemp_c"].get<double>()},
                {"condition", day["day"]["condition"]["text"].get<string>()},
                {"rain_chance", day["day"]["daily_chance_of_rain"].get<int>()}
            });
        }

        CircuitBreaker::instance().recordSuccess(service);
        weatherCache[key] = {chrono::steady_clock::now(), result};
        return result;
    } catch (const exception& e) {
        CircuitBreaker::instance().recordFailure(service);
        throw;
    }
}

// Get Amadeus token
string APIHandler::getAmadeusToken() {
    try {
        string payload = "grant_type=client_credentials&"
                        "client_id=" + AMADEUS_CLIENT_ID + "&"
                        "client_secret=" + AMADEUS_CLIENT_SECRET;

        string response = makeHttpRequest(AMADEUS_TOKEN_URL, "POST", payload);
        json j = json::parse(response);

        if (!j.contains("access_token")) {
            throw runtime_error("No access_token in response");
        }
        return j["access_token"].get<string>();
    } catch (const exception& e) {
        throw runtime_error("Error getting Amadeus token: " + string(e.what()));
    }
}

// Helper function to get IATA code using Gemini API, cached per city.
string APIHandler::getIATACode(const string& city) {
    auto cached = iataCache.find(city);
    if (cached != iataCache.end()) {
        Logger::info("IATA cache hit for " + city);
        return cached->second;
    }

    try {
        json request = {
            {"contents", {
                {
                    {"parts", {
                        {{"text", "You are an IATA airport code assistant. For the city '" + city + "', return ONLY the 3-letter IATA code of its main airport. Return just the code, nothing else. For example, if asked about New York, you would return 'JFK'."}}
                    }}
                }
            }}
        };

        string url = GEMINI_API_URL + "?key=" + GEMINI_API_KEY;
        string response = makeHttpRequest(url, "POST", request.dump());
        json responseJson = json::parse(response);

        if (!responseJson.contains("candidates") || responseJson["candidates"].empty()) {
            throw runtime_error("Invalid Gemini response structure");
        }

        string iataCode = responseJson["candidates"][0]["content"]["parts"][0]["text"].get<string>();
        iataCode.erase(remove_if(iataCode.begin(), iataCode.end(), ::isspace), iataCode.end());

        if (iataCode.length() == 3 && all_of(iataCode.begin(), iataCode.end(), ::isupper)) {
            iataCache[city] = iataCode;
            return iataCode;
        }

        throw runtime_error("Invalid IATA code format: " + iataCode);
    } catch (const exception& e) {
        throw runtime_error("Error getting IATA code: " + string(e.what()));
    }
}

// Helper function to URL encode parameters
string APIHandler::urlEncode(const string& str) {
    CURL* curl = curl_easy_init();
    string encoded;
    if (curl) {
        char* output = curl_easy_escape(curl, str.c_str(), str.length());
        if (output) {
            encoded = output;
            curl_free(output);
        }
        curl_easy_cleanup(curl);
    }
    return encoded;
}

// Search for flights
vector<Flight> APIHandler::searchFlights(const string& from, const string& to,
                                       const string& date, int passengers) {
    const string service = "amadeus";
    return retryWithBackoff<vector<Flight>>("Error in searchFlights", [&]() -> vector<Flight> {
        CircuitBreaker::instance().checkAllowed(service);
        try {
            string token = getAmadeusToken();
            string fromIATA = getIATACode(from);
            string toIATA = getIATACode(to);

            json requestBody = {
                {"currencyCode", CURRENCY_CODE},
                {"originDestinations", {{
                    {"id", "1"},
                    {"originLocationCode", fromIATA},
                    {"destinationLocationCode", toIATA},
                    {"departureDateTimeRange", {
                        {"date", date}
                    }}
                }}},
                {"travelers", {}},
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
                    {"id", to_string(i)},
                    {"travelerType", "ADULT"}
                });
            }

            string response = makeHttpRequest(AMADEUS_FLIGHT_URL, "POST", requestBody.dump(), token);
            auto flights = FlightParser::parseAmadeusFlightOffers(response, CURRENCY_CODE);
            CircuitBreaker::instance().recordSuccess(service);
            return flights;
        } catch (const exception&) {
            CircuitBreaker::instance().recordFailure(service);
            throw;
        }
    });
}

// Search for hotels. Uses Gemini structured output (responseMimeType +
// responseSchema) instead of asking the model to emit JSON in prose and
// scraping it out of markdown fences - the model is contractually bound to
// return valid JSON matching the schema, so no brittle text surgery.
vector<Hotel> APIHandler::searchHotels(const string& city, const string& checkIn,
                                       const string& checkOut, int guests) {
    const string service = "gemini";
    return retryWithBackoff<vector<Hotel>>("Error getting hotel suggestions", [&]() -> vector<Hotel> {
        CircuitBreaker::instance().checkAllowed(service);
        try {
            json request = {
                {"contents", {
                    {
                        {"parts", {
                            {{"text", "Suggest 3 good hotels to stay in " + city + " from " + checkIn + " to " + checkOut +
                                     " for " + to_string(guests) + " people."}}
                        }}
                    }
                }},
                {"generationConfig", {
                    {"responseMimeType", "application/json"},
                    {"responseSchema", {
                        {"type", "ARRAY"},
                        {"items", {
                            {"type", "OBJECT"},
                            {"properties", {
                                {"hotel_name", {{"type", "STRING"}}},
                                {"star_rating", {{"type", "NUMBER"}}},
                                {"total_stay_cost", {{"type", "NUMBER"}}},
                                {"address", {{"type", "STRING"}}}
                            }},
                            {"required", {"hotel_name", "star_rating", "total_stay_cost", "address"}}
                        }}
                    }}
                }}
            };

            string url = GEMINI_API_URL + "?key=" + GEMINI_API_KEY;
            string response = makeHttpRequest(url, "POST", request.dump());
            json responseJson = json::parse(response);

            if (!responseJson.contains("candidates") || responseJson["candidates"].empty()) {
                throw runtime_error("Invalid Gemini response structure");
            }

            string hotelText = responseJson["candidates"][0]["content"]["parts"][0]["text"];
            json hotelJson = json::parse(hotelText);

            vector<Hotel> hotels;
            for (const auto& hotel : hotelJson) {
                hotels.emplace_back(
                    hotel.at("hotel_name").get<string>(),
                    city,
                    hotel.at("total_stay_cost").get<double>(),
                    hotel.at("star_rating").get<double>(),
                    checkIn,
                    checkOut,
                    hotel.at("address").get<string>(),
                    CURRENCY_CODE
                );
            }
            CircuitBreaker::instance().recordSuccess(service);
            return hotels;
        } catch (const exception&) {
            CircuitBreaker::instance().recordFailure(service);
            throw;
        }
    });
}

// Generate itinerary, again using Gemini structured output for a
// schema-validated response instead of markdown-fence scraping.
vector<ItineraryItem> APIHandler::generateItinerary(const string& destination,
                                                  const string& startDate,
                                                  const string& endDate,
                                                  int peopleCount,
                                                  double budget,
                                                  const Hotel& selectedHotel) {
    const string service = "gemini";
    return retryWithBackoff<vector<ItineraryItem>>("Error generating itinerary", [&]() -> vector<ItineraryItem> {
        CircuitBreaker::instance().checkAllowed(service);
        try {
            tm start = {}, end = {};
            istringstream(startDate) >> get_time(&start, "%Y-%m-%d");
            istringstream(endDate) >> get_time(&end, "%Y-%m-%d");
            time_t start_time = mktime(&start);
            time_t end_time = mktime(&end);
            int num_days = static_cast<int>((end_time - start_time) / (60 * 60 * 24)) + 1;

            string prompt =
                "Create a simple " + to_string(num_days) + "-day travel itinerary for " + destination + " from " + startDate + " to " + endDate + ".\n"
                "For each day, provide a date (YYYY-MM-DD), a place to visit, what it's famous for (1-2 sentences), "
                "and how to get there (brief transportation description).";

            json request = {
                {"contents", {
                    {
                        {"parts", {
                            {{"text", prompt}}
                        }}
                    }
                }},
                {"generationConfig", {
                    {"responseMimeType", "application/json"},
                    {"responseSchema", {
                        {"type", "OBJECT"},
                        {"properties", {
                            {"itinerary", {
                                {"type", "ARRAY"},
                                {"items", {
                                    {"type", "OBJECT"},
                                    {"properties", {
                                        {"date", {{"type", "STRING"}}},
                                        {"place", {{"type", "STRING"}}},
                                        {"famous_for", {{"type", "STRING"}}},
                                        {"how_to_go", {{"type", "STRING"}}}
                                    }},
                                    {"required", {"date", "place", "famous_for", "how_to_go"}}
                                }}
                            }}
                        }},
                        {"required", {"itinerary"}}
                    }}
                }}
            };

            string url = GEMINI_API_URL + "?key=" + GEMINI_API_KEY;
            string response = makeHttpRequest(url, "POST", request.dump());
            json responseJson = json::parse(response);

            if (!responseJson.contains("candidates") || responseJson["candidates"].empty()) {
                throw runtime_error("Invalid Gemini response structure");
            }

            string itineraryText = responseJson["candidates"][0]["content"]["parts"][0]["text"];
            json itineraryJson = json::parse(itineraryText);

            vector<ItineraryItem> itinerary;

            itinerary.emplace_back("Check-in at " + selectedHotel.getName(),
                                  startDate, "14:00", "Accommodation");

            if (itineraryJson.contains("itinerary") && itineraryJson["itinerary"].is_array()) {
                for (const auto& day : itineraryJson["itinerary"]) {
                    string date = day.value("date", "");
                    string place = day.value("place", "");
                    string description = day.value("famous_for", "");
                    string transport = day.value("how_to_go", "");
                    if (!place.empty())
                        itinerary.emplace_back("Visit " + place, date, "10:00", "Sightseeing");
                    if (!description.empty())
                        itinerary.emplace_back(description, date, "10:30", "Information");
                    if (!transport.empty())
                        itinerary.emplace_back("Transportation: " + transport, date, "09:30", "Transport");
                }
            }

            itinerary.emplace_back("Check-out from " + selectedHotel.getName(),
                                  endDate, "11:00", "Accommodation");

            CircuitBreaker::instance().recordSuccess(service);
            return itinerary;
        } catch (const exception&) {
            CircuitBreaker::instance().recordFailure(service);
            throw;
        }
    });
}
