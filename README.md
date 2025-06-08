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
## Exmaple of plan prepared by TravelPlanner
```
=== Your Complete Travel Plan ===

Trip Details:
Destination: Bangalore
Dates: 2025-06-04 to 2025-06-10
People: 4
Budget: 48739.00 INR

Selected Outbound Journey:
  Leg 1: AI 2803
DEL (2025-06-04 06:25) to BLR (2025-06-04 09:25)
Price: 3085.00 INR
Available seats: 9

Selected Return Journey:
  Leg 1: AI 2804
BLR (2025-06-10 10:20) to DEL (2025-06-10 13:15)
Price: 3099.00 INR
Available seats: 9

Selected Hotel:
Hotel: The Taj West End
Rating: 5.00 stars
Total Stay Cost: 6000.00 INR
Address: 25, Race Course Road, Bengaluru, Karnataka 560001, India

Daily Itinerary:

Itinerary for Bangalore:
Date: 2025-06-04
Time: 14:00
Activity: Check-in at The Taj West End
Category: Accommodation
-------------------
Date: 2025-06-04
Time: 10:00
Activity: Visit Bangalore Palace
Category: Sightseeing
-------------------
Date: 2025-06-04
Time: 10:30
Activity: Modeled after Windsor Castle, it showcases Tudor architecture and houses historical artifacts.
Category: Information
-------------------
Date: 2025-06-04
Time: 09:30
Activity: Transportation: Auto-rickshaw or cab from city center.
Category: Transport
-------------------
Date: 2025-06-05
Time: 10:00
Activity: Visit Tipu Sultan's Summer Palace
Category: Sightseeing
-------------------
Date: 2025-06-05
Time: 10:30
Activity: A beautiful wooden structure built in the Indo-Islamic style, used as Tipu Sultan's summer retreat.
Category: Information
-------------------
Date: 2025-06-05
Time: 09:30
Activity: Transportation: Metro (nearest station is KR Market) or bus.
Category: Transport
-------------------
Date: 2025-06-06
Time: 10:00
Activity: Visit Lal Bagh Botanical Garden
Category: Sightseeing
-------------------
Date: 2025-06-06
Time: 10:30
Activity: A sprawling botanical garden with diverse flora, a glasshouse, and a historical rock formation.
Category: Information
-------------------
Date: 2025-06-06
Time: 09:30
Activity: Transportation: Metro (Lal Bagh station) or bus.
Category: Transport
-------------------
Date: 2025-06-07
Time: 10:00
Activity: Visit Vidhana Soudha
Category: Sightseeing
-------------------
Date: 2025-06-07
Time: 10:30
Activity: An impressive neo-Dravidian granite building housing the Karnataka State Legislative Assembly.
Category: Information
-------------------
Date: 2025-06-07
Time: 09:30
Activity: Transportation: Metro (nearest station is Vidhana Soudha) or bus.
Category: Transport
-------------------
Date: 2025-06-08
Time: 10:00
Activity: Visit ISKCON Temple Bangalore
Category: Sightseeing
-------------------
Date: 2025-06-08
Time: 10:30
Activity: A magnificent temple dedicated to Lord Krishna, known for its spiritual atmosphere and architectural grandeur.
Category: Information
-------------------
Date: 2025-06-08
Time: 09:30
Activity: Transportation: Bus or auto-rickshaw from city center.
Category: Transport
-------------------
Date: 2025-06-09
Time: 10:00
Activity: Visit Cubbon Park
Category: Sightseeing
-------------------
Date: 2025-06-09
Time: 10:30
Activity: A green lung in the city center, offering walking paths, gardens, and historical buildings.
Category: Information
-------------------
Date: 2025-06-09
Time: 09:30
Activity: Transportation: Metro (nearest station is Cubbon Park) or bus.
Category: Transport
-------------------
Date: 2025-06-10
Time: 10:00
Activity: Visit Commercial Street
Category: Sightseeing
-------------------
Date: 2025-06-10
Time: 10:30
Activity: A bustling street known for its diverse shopping options, from clothing to accessories.
Category: Information
-------------------
Date: 2025-06-10
Time: 09:30
Activity: Transportation: Auto-rickshaw or bus from city center.
Category: Transport
-------------------
Date: 2025-06-10
Time: 11:00
Activity: Check-out from The Taj West End
Category: Accommodation
-------------------

Total Trip Cost: 48739.00 INR

Thank you for using our service! Happy Journey!
```
