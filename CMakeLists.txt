cmake_minimum_required(VERSION 3.16)
project(TravelPlanner CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_APP "Build the CLI/server executables (requires libcurl dev headers)" ON)
option(BUILD_SERVER "Build the REST API server (requires Crow)" ON)
option(WITH_PERSISTENCE "Build SQLite-backed trip persistence" ON)

include(FetchContent)

# --- travelplanner_core: pure domain logic and parsing.
# Deliberately free of any network dependency so it builds and unit-tests
# anywhere, including CI machines without libcurl dev headers installed.
add_library(travelplanner_core STATIC
    src/flight.cpp
    src/hotel.cpp
    src/itinerary_item.cpp
    src/trip.cpp
    src/user.cpp
    src/logger.cpp
    src/date_utils.cpp
    src/flight_parser.cpp
    src/circuit_breaker.cpp
)
target_include_directories(travelplanner_core PUBLIC include)

# --- Optional SQLite persistence layer ---
if(WITH_PERSISTENCE)
    find_package(SQLite3 QUIET)
    if(SQLite3_FOUND)
        add_library(travelplanner_persistence STATIC src/trip_repository.cpp)
        target_link_libraries(travelplanner_persistence PUBLIC travelplanner_core SQLite::SQLite3)
    else()
        message(STATUS "SQLite3 not found - skipping persistence layer (set WITH_PERSISTENCE=OFF to silence)")
        set(WITH_PERSISTENCE OFF)
    endif()
endif()

# --- travelplanner_api: everything that talks to the network ---
if(BUILD_APP)
    find_package(CURL QUIET)
    if(NOT CURL_FOUND)
        message(WARNING "libcurl development headers not found - skipping executables. Unit tests will still build.")
        set(BUILD_APP OFF)
    endif()
endif()

if(BUILD_APP)
    add_library(travelplanner_api STATIC src/api_handler.cpp)
    target_link_libraries(travelplanner_api PUBLIC travelplanner_core CURL::libcurl)

    add_executable(travel_planner src/main.cpp)
    target_link_libraries(travel_planner PRIVATE travelplanner_api)
    if(WITH_PERSISTENCE)
        target_link_libraries(travel_planner PRIVATE travelplanner_persistence)
        target_compile_definitions(travel_planner PRIVATE HAVE_PERSISTENCE)
    endif()

    if(BUILD_SERVER)
        # Crow needs standalone Asio headers; fetch them rather than
        # requiring a system-wide install.
        FetchContent_Declare(
            asio
            GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
            GIT_TAG asio-1-30-2
        )
        FetchContent_MakeAvailable(asio)
        set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "" FORCE)

        FetchContent_Declare(
            Crow
            GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
            GIT_TAG v1.2.0
        )
        set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(Crow)

        add_executable(travel_planner_server src/server.cpp)
        target_link_libraries(travel_planner_server PRIVATE travelplanner_api Crow::Crow)
        if(WIN32)
            target_link_libraries(travel_planner_server PRIVATE ws2_32 mswsock)
        else()
            find_package(Threads REQUIRED)
            target_link_libraries(travel_planner_server PRIVATE Threads::Threads)
        endif()
    endif()
endif()

# --- Unit tests: link only the pure core, so they need no network stack ---
if(BUILD_TESTS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.4
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

    add_executable(travelplanner_tests
        tests/test_date_utils.cpp
        tests/test_flight_parsing.cpp
        tests/test_retry.cpp
    )
    target_link_libraries(travelplanner_tests PRIVATE travelplanner_core Catch2::Catch2WithMain)

    enable_testing()
    include(Catch)
    catch_discover_tests(travelplanner_tests)
endif()
