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
