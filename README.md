# TravelPlanner :airplane: :hotel: :partly_sunny:

TravelPlanner is a C++ console application that helps users plan trips by integrating flight/hotel searches, weather forecasts, and AI-powered itinerary generation. The project demonstrates OOP principles and API integration with Gemini AI, Amadeus Travel API, and WeatherAPI.

## Features :sparkles:
- **User Registration**: Create and manage travel profiles
- **Multi-API Integration**:
  - Flight search (Amadeus API)
  - Hotel recommendations (Gemini AI)
  - Weather forecasts (WeatherAPI)
- **Smart Itinerary Generation**: AI-powered daily plans
- **Cost Calculation**: Real-time budget tracking
- **Connection Handling**: Automatic grouping of connecting flights
- **Error Resilience**: Retry mechanisms for API failures

## Prerequisites :warning:
- C++17 compiler (GCC/MinGW on Windows)
- [libcurl](https://curl.se/windows/) installed
- API keys for:
  - [Google Gemini](https://aistudio.google.com/)
  - [Amadeus Travel APIs](https://developers.amadeus.com/)
  - [WeatherAPI](https://www.weatherapi.com/)

## Installation & Setup :wrench:
1. **Install libcurl**:
   - Windows: Download [prebuilt binaries](https://curl.se/windows/)
   - Linux/macOS: `sudo apt-get install libcurl4-openssl-dev` / `brew install curl`

2. **Configure API keys**:
```bash
mkdir config
echo '{
  "gemini": {"api_key": "YOUR_GEMINI_KEY"},
  "amadeus": {
    "client_id": "YOUR_AMADEUS_ID",
    "client_secret": "YOUR_AMADEUS_SECRET"
  },
  "weather": {"api_key": "YOUR_WEATHERAPI_KEY"}
}' > config/api_keys.json
```

## Building & Running :computer:
### Compile (adjust include/library paths as needed)
```
g++ -std=c++17 src/*.cpp -I include -I "path/to/curl/include" -L "path/to/curl/lib" -lcurl -o travel_planner
```

### Run
```
./travel_planner
```

## Sample Workflow :arrow_forward:
1. Register with username/email
2. Enter destination city (e.g., "Paris")
3. View weather forecast
4. Set travel dates (YYYY-MM-DD)
5. Choose flights:
   - Outbound journey (groups connecting flights)
   - Return journey
6. Select hotel from recommendations
7. Generate personalized itinerary
8. View complete travel plan with cost breakdown

## Project Structure :file_folder:
```
TravelPlanner/
├── config/
│   └── api_keys.json       # API credentials
├── include/                # Header files
│   ├── api_handler.hpp     # API integration
│   ├── flight.hpp          # Flight data model
│   ├── hotel.hpp           # Hotel data model
│   ├── itinerary_item.hpp  # Day plans
│   ├── trip.hpp            # Trip container
│   └── user.hpp            # User profile
├── src/                    # Implementation
│   ├── api_handler.cpp     # API logic
│   ├── flight.cpp          # Flight methods
│   ├── hotel.cpp           # Hotel methods
│   ├── itinerary_item.cpp  # Itinerary methods
│   ├── main.cpp            # Entry point
│   ├── trip.cpp            # Trip management
│   └── user.cpp            # User operations
└── travel_planner          # Compiled binary
```
