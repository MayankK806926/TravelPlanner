#ifndef TRIP_REPOSITORY_HPP
#define TRIP_REPOSITORY_HPP

#include <memory>
#include <string>
#include <vector>
#include "trip.hpp"
#include "user.hpp"

struct sqlite3;

// SQLite-backed persistence so planned trips survive process restarts.
// Schema is created on first open.
class TripRepository {
public:
    explicit TripRepository(const std::string& dbPath = "travelplanner.db");
    ~TripRepository();

    TripRepository(const TripRepository&) = delete;
    TripRepository& operator=(const TripRepository&) = delete;

    // Inserts (or reuses) a user row, returning its row id.
    long long saveUser(const std::string& username, const std::string& email);

    // Persists a trip and its itinerary items for the given user.
    long long saveTrip(long long userId, const Trip& trip);

    // Loads all trips previously saved for an email address.
    std::vector<std::unique_ptr<Trip>> loadTripsForUser(const std::string& email);

private:
    void execOrThrow(const std::string& sql);
    void initSchema();

    sqlite3* db = nullptr;
};

#endif // TRIP_REPOSITORY_HPP
