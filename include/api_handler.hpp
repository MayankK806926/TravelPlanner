#ifndef API_HANDLER_HPP
#define API_HANDLER_HPP

#include <string>
#include <vector>
#include "hotel.hpp"
#include "flight.hpp"
#include "itinerary_item.hpp"
#include "json.hpp"

using namespace std;

class FlightProvider;

class APIHandler {
public:
    // API Keys and URLs
    static string GEMINI_API_KEY;
    static string GEMINI_API_URL;
    static string AMADEUS_CLIENT_ID;
    static string AMADEUS_CLIENT_SECRET;
    static string AMADEUS_TOKEN_URL;
    static string AMADEUS_FLIGHT_URL;
    static string WEATHER_API_KEY;
    static string WEATHER_API_URL;
    static string CURRENCY_CODE;    // e.g. "INR", "USD", "EUR"
    static string FLIGHT_PROVIDER;  // "amadeus", "gemini", "mock" or "auto"

    // Initialize API keys from environment variables (falls back to
    // config/api_keys.json if the env vars are not set).
    static void initializeAPIKeys();

    // Public member functions
    static void getWeather(const string& city, int days);
    static nlohmann::json getWeatherJson(const string& city, int days);
    static vector<Flight> searchFlights(const string& from, const string& to,
                                      const string& date, int passengers);
    static vector<Hotel> searchHotels(const string& city, const string& checkIn,
                                    const string& checkOut, int guests);
    static vector<ItineraryItem> generateItinerary(const string& destination,
                                                 const string& startDate,
                                                 const string& endDate,
                                                 int peopleCount,
                                                 double budget,
                                                 const Hotel& selectedHotel);

    // Identity of the configured flight backend, for surfacing to callers.
    static string activeFlightProviderName();
    static bool flightResultsAreBookable();

    // Clears the in-memory IATA-code / weather caches (mainly for tests).
    static void clearCaches();

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static string makeHttpRequest(const string& url, const string& method = "GET",
                                const string& data = "", const string& token = "");
    static string getIATACode(const string& city);
    static string urlEncode(const string& str);

    // Lazily constructed from the current configuration, then reused.
    static FlightProvider& flightProvider();
};

#endif // API_HANDLER_HPP
