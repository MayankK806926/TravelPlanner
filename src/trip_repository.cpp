#include "trip_repository.hpp"
#include "logger.hpp"
#include <sqlite3.h>
#include <stdexcept>

namespace {
// RAII wrapper so a prepared statement is always finalized, including on
// the throwing paths below.
class Stmt {
public:
    Stmt(sqlite3* db, const std::string& sql) {
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &handle, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("Failed to prepare statement: ") + sqlite3_errmsg(db));
        }
    }
    ~Stmt() { if (handle) sqlite3_finalize(handle); }
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;

    sqlite3_stmt* get() { return handle; }

private:
    sqlite3_stmt* handle = nullptr;
};
}

TripRepository::TripRepository(const std::string& dbPath) {
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::string err = db ? sqlite3_errmsg(db) : "unknown error";
        if (db) sqlite3_close(db);
        db = nullptr;
        throw std::runtime_error("Failed to open database " + dbPath + ": " + err);
    }
    initSchema();
    Logger::info("Trip repository opened at " + dbPath);
}

TripRepository::~TripRepository() {
    if (db) sqlite3_close(db);
}

void TripRepository::execOrThrow(const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string message = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("SQL error: " + message);
    }
}

void TripRepository::initSchema() {
    execOrThrow(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT NOT NULL,"
        "  email TEXT NOT NULL UNIQUE"
        ");"
    );
    execOrThrow(
        "CREATE TABLE IF NOT EXISTS trips ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  user_id INTEGER NOT NULL REFERENCES users(id),"
        "  destination TEXT NOT NULL,"
        "  start_date TEXT NOT NULL,"
        "  end_date TEXT NOT NULL,"
        "  people_count INTEGER NOT NULL,"
        "  budget REAL NOT NULL,"
        "  currency TEXT NOT NULL"
        ");"
    );
    execOrThrow(
        "CREATE TABLE IF NOT EXISTS itinerary_items ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  trip_id INTEGER NOT NULL REFERENCES trips(id),"
        "  activity TEXT NOT NULL,"
        "  date TEXT NOT NULL,"
        "  time TEXT NOT NULL,"
        "  category TEXT NOT NULL"
        ");"
    );
}

long long TripRepository::saveUser(const std::string& username, const std::string& email) {
    {
        Stmt insert(db, "INSERT OR IGNORE INTO users (username, email) VALUES (?, ?);");
        sqlite3_bind_text(insert.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert.get(), 2, email.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(insert.get()) != SQLITE_DONE) {
            throw std::runtime_error(std::string("Failed to save user: ") + sqlite3_errmsg(db));
        }
    }

    Stmt select(db, "SELECT id FROM users WHERE email = ?;");
    sqlite3_bind_text(select.get(), 1, email.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(select.get()) != SQLITE_ROW) {
        throw std::runtime_error("Failed to look up saved user id");
    }
    return sqlite3_column_int64(select.get(), 0);
}

long long TripRepository::saveTrip(long long userId, const Trip& trip) {
    long long tripId = 0;
    {
        Stmt insert(db,
            "INSERT INTO trips (user_id, destination, start_date, end_date, people_count, budget, currency) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);");
        sqlite3_bind_int64(insert.get(), 1, userId);
        std::string destination = trip.getDestination();
        std::string startDate = trip.getStartDate();
        std::string endDate = trip.getEndDate();
        std::string currency = trip.getCurrency();
        sqlite3_bind_text(insert.get(), 2, destination.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert.get(), 3, startDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert.get(), 4, endDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(insert.get(), 5, trip.getPeopleCount());
        sqlite3_bind_double(insert.get(), 6, trip.getBudget());
        sqlite3_bind_text(insert.get(), 7, currency.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(insert.get()) != SQLITE_DONE) {
            throw std::runtime_error(std::string("Failed to save trip: ") + sqlite3_errmsg(db));
        }
        tripId = sqlite3_last_insert_rowid(db);
    }

    for (const auto& item : trip.getItinerary()) {
        Stmt insertItem(db,
            "INSERT INTO itinerary_items (trip_id, activity, date, time, category) VALUES (?, ?, ?, ?, ?);");
        std::string activity = item.getActivity();
        std::string date = item.getDate();
        std::string time = item.getTime();
        std::string category = item.getCategory();
        sqlite3_bind_int64(insertItem.get(), 1, tripId);
        sqlite3_bind_text(insertItem.get(), 2, activity.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertItem.get(), 3, date.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertItem.get(), 4, time.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insertItem.get(), 5, category.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(insertItem.get()) != SQLITE_DONE) {
            throw std::runtime_error(std::string("Failed to save itinerary item: ") + sqlite3_errmsg(db));
        }
    }

    Logger::info("Saved trip to " + trip.getDestination() + " (id " + std::to_string(tripId) + ")");
    return tripId;
}

std::vector<std::unique_ptr<Trip>> TripRepository::loadTripsForUser(const std::string& email) {
    std::vector<std::unique_ptr<Trip>> trips;

    Stmt select(db,
        "SELECT t.id, t.destination, t.start_date, t.end_date, t.people_count, t.budget, t.currency "
        "FROM trips t JOIN users u ON u.id = t.user_id WHERE u.email = ? ORDER BY t.id;");
    sqlite3_bind_text(select.get(), 1, email.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(select.get()) == SQLITE_ROW) {
        long long tripId = sqlite3_column_int64(select.get(), 0);
        auto textAt = [&](int col) {
            const unsigned char* raw = sqlite3_column_text(select.get(), col);
            return raw ? std::string(reinterpret_cast<const char*>(raw)) : std::string();
        };

        auto trip = std::make_unique<Trip>(
            textAt(1), textAt(2), textAt(3),
            sqlite3_column_int(select.get(), 4),
            sqlite3_column_double(select.get(), 5),
            textAt(6)
        );

        Stmt items(db, "SELECT activity, date, time, category FROM itinerary_items WHERE trip_id = ? ORDER BY id;");
        sqlite3_bind_int64(items.get(), 1, tripId);
        while (sqlite3_step(items.get()) == SQLITE_ROW) {
            auto itemText = [&](int col) {
                const unsigned char* raw = sqlite3_column_text(items.get(), col);
                return raw ? std::string(reinterpret_cast<const char*>(raw)) : std::string();
            };
            trip->addItineraryItem(ItineraryItem(itemText(0), itemText(1), itemText(2), itemText(3)));
        }

        trips.push_back(std::move(trip));
    }

    return trips;
}
