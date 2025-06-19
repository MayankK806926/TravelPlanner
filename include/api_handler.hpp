#ifndef API_HANDLER_HPP
#define API_HANDLER_HPP

#include <string>
#include <vector>
#include "hotel.hpp"
#include "flight.hpp"
#include "itinerary_item.hpp"

using namespace std;

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

    // Initialize API keys
    static void initializeAPIKeys();

    // Public member functions
    static void getWeather(const string& city, int days);
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

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static string makeHttpRequest(const string& url, const string& method = "GET",
                                const string& data = "", const string& token = "");
    static vector<Flight> parseAmadeusFlightOffers(const string& response);
    static string getAmadeusToken();
    static string getIATACode(const string& city);
    static string urlEncode(const string& str);
};

#endif // API_HANDLER_HPP


//REST_APT_SERVER
/*
#ifndef API_HANDLER_HPP
#define API_HANDLER_HPP

#include <string>
#include <vector>
#include "hotel.hpp"
#include "flight.hpp"
#include "itinerary_item.hpp"
#include "json.hpp" // Include the JSON library header

using namespace std;

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

    // Initialize API keys
    static void initializeAPIKeys();

    // Public member functions
    static void getWeather(const string& city, int days);
    static nlohmann::json getWeatherJson(const string& city, int days); // New method for weather data as JSON
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

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static string makeHttpRequest(const string& url, const string& method = "GET",
                                const string& data = "", const string& token = "");
    static vector<Flight> parseAmadeusFlightOffers(const string& response);
    static string getAmadeusToken();
    static string getIATACode(const string& city);
    static string urlEncode(const string& str);
};

#endif // API_HANDLER_HPP
*/
