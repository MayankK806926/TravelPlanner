# TravelPlanner :airplane: :hotel: :partly_sunny:

TravelPlanner is a C++17 travel-planning application that integrates flight
and hotel search, weather forecasts, and AI-generated itineraries. It ships
as both an interactive CLI and a REST API server with a small web frontend.

## Features :sparkles:
- **Multi-API integration**: flights (pluggable provider), hotels + itineraries (Gemini structured output), weather (WeatherAPI)
- **Swappable flight backends**: Amadeus, an AI estimator, or an offline mock - selected by configuration
- **Concurrent lookups**: independent flight/hotel searches run in parallel via `std::async`
- **Resilience**: exponential-backoff retry plus a per-service circuit breaker
- **Caching**: in-memory IATA-code and weather caches cut latency and API spend
- **Persistence**: trips and itineraries stored in SQLite, surviving restarts
- **Multi-currency**: currency configurable via `CURRENCY_CODE`, not hardcoded
- **Two frontends**: interactive CLI and a Crow-based REST API with a demo web UI

## Architecture :building_construction:

The build is split so that pure logic is testable without a network stack:

| Target | Contents | Dependencies |
|---|---|---|
| `travelplanner_core` | Domain models, flight-offer parsing, date validation, logging, circuit breaker | none |
| `travelplanner_persistence` | SQLite trip/user repository | SQLite3 |
| `travelplanner_api` | HTTP client, Amadeus/Gemini/Weather integration | libcurl |
| `travel_planner` | Interactive CLI | above |
| `travel_planner_server` | REST API + web demo | above + Crow |
| `travelplanner_tests` | Catch2 unit tests | `travelplanner_core` only |

Unit tests link only the pure core, so CI can run them without libcurl.

## Prerequisites :warning:
- C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.16+
- libcurl development headers (for the executables)
- SQLite3 development headers (for persistence; optional)

Crow and Catch2 are fetched automatically by CMake.

## Configuration :key:

API credentials are read from **environment variables**. Never commit keys.

```bash
export GEMINI_API_KEY=your_gemini_key
export AMADEUS_CLIENT_ID=your_amadeus_id
export AMADEUS_CLIENT_SECRET=your_amadeus_secret
export WEATHER_API_KEY=your_weatherapi_key
export CURRENCY_CODE=INR       # optional, defaults to INR
export FLIGHT_PROVIDER=auto    # optional: auto | amadeus | gemini | mock
```

For local development you may instead copy `config/api_keys.json.example` to
`config/api_keys.json` and fill it in - that file is gitignored and is only
used as a fallback when the environment variables are unset.

Get keys from [Google Gemini](https://aistudio.google.com/),
[Amadeus](https://developers.amadeus.com/), and
[WeatherAPI](https://www.weatherapi.com/).

### Flight providers :airplane:

Flight inventory is the one dependency with no reliably available free tier,
so it sits behind a `FlightProvider` interface and is chosen by configuration:

| `FLIGHT_PROVIDER` | Backend | Requires | Bookable? |
|---|---|---|---|
| `amadeus` | Amadeus Self-Service GDS | `AMADEUS_CLIENT_ID` + `AMADEUS_CLIENT_SECRET` | **Yes** - real inventory |
| `gemini` | AI-generated route estimates | `GEMINI_API_KEY` | No - estimates only |
| `mock` | Deterministic offline data | nothing | No - synthetic |
| `auto` (default) | First of the above that is configured | - | depends |

> **Important:** only the `amadeus` backend returns real, bookable flights.
> The `gemini` and `mock` backends produce representative options useful for
> planning and budgeting, but their flight numbers and fares are **not real**
> and cannot be booked. This is surfaced everywhere it matters: each result
> carries a `source` field, the REST API returns a `bookable` flag, the CLI
> tags estimates inline, and the web UI shows a warning banner.
>
> If you do not have Amadeus credentials, the app still runs end to end -
> it just falls back to estimated flight data.

## Building :computer:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Useful options: `-DBUILD_TESTS=OFF`, `-DBUILD_SERVER=OFF`, `-DBUILD_APP=OFF`
(tests only), `-DWITH_PERSISTENCE=OFF`.

## Running tests :test_tube:

```bash
cd build && ctest --output-on-failure
```

## Running :arrow_forward:

Interactive CLI:

```bash
./build/travel_planner
```

REST API server plus web demo (defaults to port 8080, override with `PORT`):

```bash
./build/travel_planner_server
```

Then open <http://localhost:8080> for the demo UI, or call the API directly:

| Method | Endpoint | Parameters |
|---|---|---|
| GET/POST | `/weather` | `city`, `days` |
| GET/POST | `/flights` | `from`, `to`, `date`, `passengers` |
| GET/POST | `/hotels` | `city`, `checkin`, `checkout`, `guests` |
| GET/POST | `/itinerary` | `destination`, `start`, `end`, `people`, `budget`, `hotel` |

```bash
curl "http://localhost:8080/weather?city=Bangalore&days=3"
```

`/flights` wraps its results with the provenance of the data, so a client can
tell live inventory from an estimate:

```json
{
  "provider": "mock",
  "bookable": false,
  "flights": [
    {
      "airline": "AI", "flightNumber": "1000",
      "departureAirport": "DEL", "arrivalAirport": "BAN",
      "price": 3705.0, "currency": "INR",
      "availableSeats": 6, "source": "mock"
    }
  ]
}
```

Run the server from the repository root so it can find `web/index.html`.

## Sample CLI workflow :arrow_forward:
1. Register with username/email
2. Enter destination city and view the weather forecast
3. Set travel dates (validated against the real calendar)
4. Pick outbound and return journeys (connecting flights grouped automatically)
5. Select a hotel from AI-generated recommendations
6. Review the generated itinerary and total cost - saved to SQLite on exit

## Project Structure :file_folder:
```
TravelPlanner/
├── .github/workflows/ci.yml   # Build + test on Linux and Windows
├── CMakeLists.txt
├── config/
│   └── api_keys.json.example  # Template; real file is gitignored
├── include/                   # Public headers
├── src/                       # Implementation
├── tests/                     # Catch2 unit tests
└── web/index.html             # Demo frontend for the REST API
```
