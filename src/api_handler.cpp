#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "api_handler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
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

// Initialize API keys from config file
void APIHandler::initializeAPIKeys() {
    try {
        ifstream config_file("config/api_keys.json");
        if (!config_file.is_open()) {
            throw runtime_error("Could not open config/api_keys.json");
        }

        json config = json::parse(config_file);
        
        GEMINI_API_KEY = config["gemini"]["api_key"];
        AMADEUS_CLIENT_ID = config["amadeus"]["client_id"];
        AMADEUS_CLIENT_SECRET = config["amadeus"]["client_secret"];
        WEATHER_API_KEY = config["weather"]["api_key"];
        
        // cout << "API keys loaded successfully.\n";
    } catch (const exception& e) {
        throw runtime_error("Error loading API keys: " + string(e.what()));
    }
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
            
            // Set appropriate content type based on the URL and endpoint
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

// Get weather forecast
void APIHandler::getWeather(const string& city, int days) {
    string url = WEATHER_API_URL + "/forecast.json?key=" + WEATHER_API_KEY + 
                "&q=" + city + "&days=" + to_string(days);
    
    string response = makeHttpRequest(url);
    json responseJson = json::parse(response);

    cout << "\nWeather forecast for " << city << ":\n";
    for (const auto& day : responseJson["forecast"]["forecastday"]) {
        cout << "Date: " << day["date"].get<string>() << "\n"
             << "Max temp: " << day["day"]["maxtemp_c"].get<double>() << " C\n"
             << "Min temp: " << day["day"]["mintemp_c"].get<double>() << " C\n"
             << "Condition: " << day["day"]["condition"]["text"].get<string>() << "\n"
             << "Rain chance: " << day["day"]["daily_chance_of_rain"].get<int>() << "%\n\n";
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

// Helper function to get IATA code using Gemini API
string APIHandler::getIATACode(const string& city) {
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
    const int maxRetries = 3;
    int attempt = 0;
    while (attempt < maxRetries) {
        try {
            string token = getAmadeusToken();
            string fromIATA = getIATACode(from);
            string toIATA = getIATACode(to);

            json requestBody = {
                {"currencyCode", "INR"},
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
            return parseAmadeusFlightOffers(response);
        } catch (const exception& e) {
            std::string errMsg = e.what();
            if (errMsg.find("HTTP error 503") != std::string::npos && attempt < maxRetries - 1) {
                int delay = 2 * (attempt + 1);
                std::cerr << "Amadeus or Gemini API overloaded (503). Retrying in " << delay << " seconds...\n";
#ifdef _WIN32
                Sleep(delay * 1000);
#else
                sleep(delay);
#endif
                attempt++;
                continue;
            } else {
                throw runtime_error("Error in searchFlights: " + errMsg);
            }
        }
    }
    throw runtime_error("Error in searchFlights: Amadeus or Gemini API overloaded after multiple attempts.");
}

// Search for hotels
vector<Hotel> APIHandler::searchHotels(const string& city, const string& checkIn,
                                       const string& checkOut, int guests) {
    const int maxRetries = 3;
    int attempt = 0;
    while (attempt < maxRetries) {
        try {
            json request = {
                {"contents", {
                    {
                        {"parts", {
                            {{"text", "Suggest 3 good hotels to stay in " + city + " from " + checkIn + " to " + checkOut + 
                                     " for " + to_string(guests) + " people. "
                                     "For each hotel, provide ONLY the following fields: hotel_name, star_rating, total_stay_cost, and address. "
                                     "Return ONLY a valid JSON array with 3 hotel objects. Do NOT include any extra text, notes, or explanations. Do not use Markdown or backticks."}}
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

            string hotelText = responseJson["candidates"][0]["content"]["parts"][0]["text"];

            /// Check if hotelText starts with "```"
            if (hotelText.compare(0, 3, "```") == 0) {
                size_t first = hotelText.find("```");
                size_t last = hotelText.rfind("```");
                if (first != string::npos && last != string::npos && last > first) {
                    hotelText = hotelText.substr(first + 3, last - first - 3);
                    // Check if hotelText starts with "json\n"
                    if (hotelText.compare(0, 5, "json\n") == 0) {
                        hotelText = hotelText.substr(5);
                    }
                }
            }

            // Optional: trim leading/trailing whitespace
            hotelText.erase(0, hotelText.find_first_not_of(" \n\r\t"));
            hotelText.erase(hotelText.find_last_not_of(" \n\r\t") + 1);

            json hotelJson = json::parse(hotelText);

            vector<Hotel> hotels;
            for (const auto& hotel : hotelJson) {
                hotels.emplace_back(
                    hotel["hotel_name"].get<string>(),
                    city,
                    hotel["total_stay_cost"].get<double>(),
                    hotel["star_rating"].get<double>(),
                    checkIn,
                    checkOut,
                    hotel["address"].get<string>()
                );
            }
            return hotels;
        } catch (const exception& e) {
            std::string errMsg = e.what();
            // Check for HTTP 503 error (model overloaded)
            if (errMsg.find("HTTP error 503") != std::string::npos && attempt < maxRetries - 1) {
                int delay = 2 * (attempt + 1); // Exponential backoff: 2s, 4s, ...
                std::cerr << "Gemini API overloaded (503). Retrying in " << delay << " seconds...\n";
#ifdef _WIN32
                Sleep(delay * 1000);
#else
                sleep(delay);
#endif
                attempt++;
                continue;
            } else {
                throw runtime_error("Error getting hotel suggestions: " + errMsg);
            }
        }
    }
    throw runtime_error("Error getting hotel suggestions: Gemini API overloaded after multiple attempts.");
}

// Parse flight offers
vector<Flight> APIHandler::parseAmadeusFlightOffers(const string& response) {
    vector<Flight> flights;
    try {
        json responseJson = json::parse(response);
        
        if (!responseJson.contains("data")) {
            if (responseJson.contains("errors")) {
                throw runtime_error(responseJson["errors"].dump(2));
            }
            return flights;
        }

        auto offers = responseJson["data"];
        sort(offers.begin(), offers.end(), 
             [](const json& a, const json& b) {
                 return stod(a["price"]["total"].get<string>()) < 
                        stod(b["price"]["total"].get<string>());
             });

        size_t numOffers = min(size_t(10), offers.size());
        for (size_t i = 0; i < numOffers; ++i) {
            try {
                const auto& offer = offers[i];
                const auto& itinerary = offer["itineraries"][0];
                
                for (const auto& segment : itinerary["segments"]) {
                    string airline = segment["carrierCode"].get<string>();
                    string flightNumber = segment["number"].get<string>();
                    string depAirport = segment["departure"]["iataCode"].get<string>();
                    string arrAirport = segment["arrival"]["iataCode"].get<string>();
                    string depTime = segment["departure"]["at"].get<string>();
                    string arrTime = segment["arrival"]["at"].get<string>();
                    double price = stod(offer["price"]["total"].get<string>());
                    int availableSeats = offer.value("numberOfBookableSeats", 1);

                    tm departure = {}, arrival = {};
                    istringstream(depTime) >> get_time(&departure, "%Y-%m-%dT%H:%M:%S");
                    istringstream(arrTime) >> get_time(&arrival, "%Y-%m-%dT%H:%M:%S");

                    flights.emplace_back(airline, flightNumber, depAirport, arrAirport,
                                       departure, arrival, price, availableSeats);
                }
            } catch (const exception& e) {
                continue;
            }
        }
    } catch (const exception& e) {
        throw runtime_error("Error parsing flight offers: " + string(e.what()));
    }
    
    return flights;
}

// Generate itinerary
vector<ItineraryItem> APIHandler::generateItinerary(const string& destination,
                                                  const string& startDate,
                                                  const string& endDate,
                                                  int peopleCount,
                                                  double budget,
                                                  const Hotel& selectedHotel) {
    const int maxRetries = 3;
    int attempt = 0;
    while (attempt < maxRetries) {
        try {
            // Calculate number of days
            tm start = {}, end = {};
            istringstream(startDate) >> get_time(&start, "%Y-%m-%d");
            istringstream(endDate) >> get_time(&end, "%Y-%m-%d");
            time_t start_time = mktime(&start);
            time_t end_time = mktime(&end);
            int num_days = (end_time - start_time) / (60 * 60 * 24) + 1;

            // Build prompt string with proper escaping
            string prompt =
                "Create a simple " + to_string(num_days) + "-day travel itinerary for " + destination + " from " + startDate + " to " + endDate + ".\n"
                "For each day, provide:\n"
                "1. Date (in YYYY-MM-DD format)\n"
                "2. Place name to visit\n"
                "3. What it is famous for (1-2 sentences)\n"
                "4. How to get there (very brief transportation description)\n\n"
                "Format the response as a clean JSON object with this structure:\n"
                "{\n"
                "    \"destination\": \"" + destination + "\",\n"
                "    \"itinerary\": [\n"
                "        {\n"
                "            \"date\": \"YYYY-MM-DD\",\n"
                "            \"place\": \"Place Name\",\n"
                "            \"famous_for\": \"Brief description\",\n"
                "            \"how_to_go\": \"Brief transportation info\"\n"
                "        },\n"
                "        ...more days\n"
                "    ]\n"
                "}\n"
                "Keep each description very concise (1 sentence each for famous_for and how_to_go).";

            json request = {
                {"contents", {
                    {
                        {"parts", {
                            {{"text", prompt}}
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

            string itineraryText = responseJson["candidates"][0]["content"]["parts"][0]["text"];

            // Clean up Gemini response (remove backticks and optional "json\n")
            if (itineraryText.compare(0, 3, "```") == 0) {
                size_t first = itineraryText.find("```");
                size_t last = itineraryText.rfind("```");
                if (first != string::npos && last != string::npos && last > first) {
                    itineraryText = itineraryText.substr(first + 3, last - first - 3);
                    if (itineraryText.compare(0, 5, "json\n") == 0) {
                        itineraryText = itineraryText.substr(5);
                    }
                }
            }

            // Trim whitespace
            itineraryText.erase(0, itineraryText.find_first_not_of(" \n\r\t"));
            itineraryText.erase(itineraryText.find_last_not_of(" \n\r\t") + 1);

            json itineraryJson = json::parse(itineraryText);

            vector<ItineraryItem> itinerary;

            // Add hotel check-in on first day
            itinerary.emplace_back("Check-in at " + selectedHotel.getName(), 
                                  startDate, "14:00", "Accommodation");

            // Add daily activities from Gemini response (use correct field names)
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

            // Add hotel check-out on last day
            itinerary.emplace_back("Check-out from " + selectedHotel.getName(), 
                                  endDate, "11:00", "Accommodation");
            
            return itinerary;
        } catch (const exception& e) {
            std::string errMsg = e.what();
            if (errMsg.find("HTTP error 503") != std::string::npos && attempt < maxRetries - 1) {
                int delay = 2 * (attempt + 1);
                std::cerr << "Gemini API overloaded (503). Retrying in " << delay << " seconds...\n";
#ifdef _WIN32
                Sleep(delay * 1000);
#else
                sleep(delay);
#endif
                attempt++;
                continue;
            } else {
                throw runtime_error("Error generating itinerary: " + errMsg);
            }
        }
    }
    throw runtime_error("Error generating itinerary: Gemini API overloaded after multiple attempts.");
}




















//REST_API_SERVER_CODE
/*
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "api_handler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
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

// Initialize API keys from config file
void APIHandler::initializeAPIKeys() {
    try {
        ifstream config_file("config/api_keys.json");
        if (!config_file.is_open()) {
            throw runtime_error("Could not open config/api_keys.json");
        }

        json config = json::parse(config_file);
        
        GEMINI_API_KEY = config["gemini"]["api_key"];
        AMADEUS_CLIENT_ID = config["amadeus"]["client_id"];
        AMADEUS_CLIENT_SECRET = config["amadeus"]["client_secret"];
        WEATHER_API_KEY = config["weather"]["api_key"];
        
        // cout << "API keys loaded successfully.\n";
    } catch (const exception& e) {
        throw runtime_error("Error loading API keys: " + string(e.what()));
    }
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
            
            // Set appropriate content type based on the URL and endpoint
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

// Get weather forecast
void APIHandler::getWeather(const string& city, int days) {
    string url = WEATHER_API_URL + "/forecast.json?key=" + WEATHER_API_KEY + 
                "&q=" + city + "&days=" + to_string(days);
    
    string response = makeHttpRequest(url);
    json responseJson = json::parse(response);

    cout << "\nWeather forecast for " << city << ":\n";
    for (const auto& day : responseJson["forecast"]["forecastday"]) {
        cout << "Date: " << day["date"].get<string>() << "\n"
             << "Max temp: " << day["day"]["maxtemp_c"].get<double>() << " C\n"
             << "Min temp: " << day["day"]["mintemp_c"].get<double>() << " C\n"
             << "Condition: " << day["day"]["condition"]["text"].get<string>() << "\n"
             << "Rain chance: " << day["day"]["daily_chance_of_rain"].get<int>() << "%\n\n";
    }
}

// Get weather forecast and return as JSON
json APIHandler::getWeatherJson(const string& city, int days) {
    string url = WEATHER_API_URL + "/forecast.json?key=" + WEATHER_API_KEY + 
                "&q=" + city + "&days=" + to_string(days);
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
    return result;
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

// Helper function to get IATA code using Gemini API
string APIHandler::getIATACode(const string& city) {
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
    const int maxRetries = 3;
    int attempt = 0;
    while (attempt < maxRetries) {
        try {
            string token = getAmadeusToken();
            string fromIATA = getIATACode(from);
            string toIATA = getIATACode(to);

            json requestBody = {
                {"currencyCode", "INR"},
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
            return parseAmadeusFlightOffers(response);
        } catch (const exception& e) {
            std::string errMsg = e.what();
            if (errMsg.find("HTTP error 503") != std::string::npos && attempt < maxRetries - 1) {
                int delay = 2 * (attempt + 1);
                std::cerr << "Amadeus or Gemini API overloaded (503). Retrying in " << delay << " seconds...\n";
#ifdef _WIN32
                Sleep(delay * 1000);
#else
                sleep(delay);
#endif
                attempt++;
                continue;
            } else {
                throw runtime_error("Error in searchFlights: " + errMsg);
            }
        }
    }
    throw runtime_error("Error in searchFlights: Amadeus or Gemini API overloaded after multiple attempts.");
}

// Search for hotels
vector<Hotel> APIHandler::searchHotels(const string& city, const string& checkIn,
                                       const string& checkOut, int guests) {
    const int maxRetries = 3;
    int attempt = 0;
    while (attempt < maxRetries) {
        try {
            json request = {
                {"contents", {
                    {
                        {"parts", {
                            {{"text", "Suggest 3 good hotels to stay in " + city + " from " + checkIn + " to " + checkOut + 
                                     " for " + to_string(guests) + " people. "
                                     "For each hotel, provide ONLY the following fields: hotel_name, star_rating, total_stay_cost, and address. "
                                     "Return ONLY a valid JSON array with 3 hotel objects. Do NOT include any extra text, notes, or explanations. Do not use Markdown or backticks."}}
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

            string hotelText = responseJson["candidates"][0]["content"]["parts"][0]["text"];

            /// Check if hotelText starts with "```"
            if (hotelText.compare(0, 3, "```") == 0) {
                size_t first = hotelText.find("```");
                size_t last = hotelText.rfind("```");
                if (first != string::npos && last != string::npos && last > first) {
                    hotelText = hotelText.substr(first + 3, last - first - 3);
                    // Check if hotelText starts with "json\n"
                    if (hotelText.compare(0, 5, "json\n") == 0) {
                        hotelText = hotelText.substr(5);
                    }
                }
            }

            // Optional: trim leading/trailing whitespace
            hotelText.erase(0, hotelText.find_first_not_of(" \n\r\t"));
            hotelText.erase(hotelText.find_last_not_of(" \n\r\t") + 1);

            json hotelJson = json::parse(hotelText);

            vector<Hotel> hotels;
            for (const auto& hotel : hotelJson) {
                hotels.emplace_back(
                    hotel["hotel_name"].get<string>(),
                    city,
                    hotel["total_stay_cost"].get<double>(),
                    hotel["star_rating"].get<double>(),
                    checkIn,
                    checkOut,
                    hotel["address"].get<string>()
                );
            }
            return hotels;
        } catch (const exception& e) {
            std::string errMsg = e.what();
            // Check for HTTP 503 error (model overloaded)
            if (errMsg.find("HTTP error 503") != std::string::npos && attempt < maxRetries - 1) {
                int delay = 2 * (attempt + 1); // Exponential backoff: 2s, 4s, ...
                std::cerr << "Gemini API overloaded (503). Retrying in " << delay << " seconds...\n";
#ifdef _WIN32
                Sleep(delay * 1000);
#else
                sleep(delay);
#endif
                attempt++;
                continue;
            } else {
                throw runtime_error("Error getting hotel suggestions: " + errMsg);
            }
        }
    }
    throw runtime_error("Error getting hotel suggestions: Gemini API overloaded after multiple attempts.");
}

// Parse flight offers
vector<Flight> APIHandler::parseAmadeusFlightOffers(const string& response) {
    vector<Flight> flights;
    try {
        json responseJson = json::parse(response);
        
        if (!responseJson.contains("data")) {
            if (responseJson.contains("errors")) {
                throw runtime_error(responseJson["errors"].dump(2));
            }
            return flights;
        }

        auto offers = responseJson["data"];
        sort(offers.begin(), offers.end(), 
             [](const json& a, const json& b) {
                 return stod(a["price"]["total"].get<string>()) < 
                        stod(b["price"]["total"].get<string>());
             });

        size_t numOffers = min(size_t(10), offers.size());
        for (size_t i = 0; i < numOffers; ++i) {
            try {
                const auto& offer = offers[i];
                const auto& itinerary = offer["itineraries"][0];
                
                for (const auto& segment : itinerary["segments"]) {
                    string airline = segment["carrierCode"].get<string>();
                    string flightNumber = segment["number"].get<string>();
                    string depAirport = segment["departure"]["iataCode"].get<string>();
                    string arrAirport = segment["arrival"]["iataCode"].get<string>();
                    string depTime = segment["departure"]["at"].get<string>();
                    string arrTime = segment["arrival"]["at"].get<string>();
                    double price = stod(offer["price"]["total"].get<string>());
                    int availableSeats = offer.value("numberOfBookableSeats", 1);

                    tm departure = {}, arrival = {};
                    istringstream(depTime) >> get_time(&departure, "%Y-%m-%dT%H:%M:%S");
                    istringstream(arrTime) >> get_time(&arrival, "%Y-%m-%dT%H:%M:%S");

                    flights.emplace_back(airline, flightNumber, depAirport, arrAirport,
                                       departure, arrival, price, availableSeats);
                }
            } catch (const exception& e) {
                continue;
            }
        }
    } catch (const exception& e) {
        throw runtime_error("Error parsing flight offers: " + string(e.what()));
    }
    
    return flights;
}

// Generate itinerary
vector<ItineraryItem> APIHandler::generateItinerary(const string& destination,
                                                  const string& startDate,
                                                  const string& endDate,
                                                  int peopleCount,
                                                  double budget,
                                                  const Hotel& selectedHotel) {
    const int maxRetries = 3;
    int attempt = 0;
    while (attempt < maxRetries) {
        try {
            // Calculate number of days
            tm start = {}, end = {};
            istringstream(startDate) >> get_time(&start, "%Y-%m-%d");
            istringstream(endDate) >> get_time(&end, "%Y-%m-%d");
            time_t start_time = mktime(&start);
            time_t end_time = mktime(&end);
            int num_days = (end_time - start_time) / (60 * 60 * 24) + 1;

            // Build prompt string with proper escaping
            string prompt =
                "Create a simple " + to_string(num_days) + "-day travel itinerary for " + destination + " from " + startDate + " to " + endDate + ".\n"
                "For each day, provide:\n"
                "1. Date (in YYYY-MM-DD format)\n"
                "2. Place name to visit\n"
                "3. What it is famous for (1-2 sentences)\n"
                "4. How to get there (very brief transportation description)\n\n"
                "Format the response as a clean JSON object with this structure:\n"
                "{\n"
                "    \"destination\": \"" + destination + "\",\n"
                "    \"itinerary\": [\n"
                "        {\n"
                "            \"date\": \"YYYY-MM-DD\",\n"
                "            \"place\": \"Place Name\",\n"
                "            \"famous_for\": \"Brief description\",\n"
                "            \"how_to_go\": \"Brief transportation info\"\n"
                "        },\n"
                "        ...more days\n"
                "    ]\n"
                "}\n"
                "Keep each description very concise (1 sentence each for famous_for and how_to_go).";

            json request = {
                {"contents", {
                    {
                        {"parts", {
                            {{"text", prompt}}
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

            string itineraryText = responseJson["candidates"][0]["content"]["parts"][0]["text"];

            // Clean up Gemini response (remove backticks and optional "json\n")
            if (itineraryText.compare(0, 3, "```") == 0) {
                size_t first = itineraryText.find("```");
                size_t last = itineraryText.rfind("```");
                if (first != string::npos && last != string::npos && last > first) {
                    itineraryText = itineraryText.substr(first + 3, last - first - 3);
                    if (itineraryText.compare(0, 5, "json\n") == 0) {
                        itineraryText = itineraryText.substr(5);
                    }
                }
            }

            // Trim whitespace
            itineraryText.erase(0, itineraryText.find_first_not_of(" \n\r\t"));
            itineraryText.erase(itineraryText.find_last_not_of(" \n\r\t") + 1);

            json itineraryJson = json::parse(itineraryText);

            vector<ItineraryItem> itinerary;

            // Add hotel check-in on first day
            itinerary.emplace_back("Check-in at " + selectedHotel.getName(), 
                                  startDate, "14:00", "Accommodation");

            // Add daily activities from Gemini response (use correct field names)
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

            // Add hotel check-out on last day
            itinerary.emplace_back("Check-out from " + selectedHotel.getName(), 
                                  endDate, "11:00", "Accommodation");
            
            return itinerary;
        } catch (const exception& e) {
            std::string errMsg = e.what();
            if (errMsg.find("HTTP error 503") != std::string::npos && attempt < maxRetries - 1) {
                int delay = 2 * (attempt + 1);
                std::cerr << "Gemini API overloaded (503). Retrying in " << delay << " seconds...\n";
#ifdef _WIN32
                Sleep(delay * 1000);
#else
                sleep(delay);
#endif
                attempt++;
                continue;
            } else {
                throw runtime_error("Error generating itinerary: " + errMsg);
            }
        }
    }
    throw runtime_error("Error generating itinerary: Gemini API overloaded after multiple attempts.");
}
*/
