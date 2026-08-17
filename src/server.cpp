#define CROW_MAIN
#include <crow.h>
#include <crow/middlewares/cors.h>
#include "api_handler.hpp"
#include "logger.hpp"
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace std;
using json = nlohmann::json;

namespace {

// Flight responses carry the backing provider and whether the results are
// real bookable inventory, so a client can never mistake an estimate for a
// flight it can actually buy.
json serializeFlights(const std::vector<Flight>& flights) {
    json arr = json::array();
    for (const auto& f : flights) {
        arr.push_back({
            {"airline", f.getAirline()},
            {"flightNumber", f.getFlightNumber()},
            {"departureAirport", f.getDepartureAirport()},
            {"arrivalAirport", f.getArrivalAirport()},
            {"price", f.getPrice()},
            {"currency", f.getCurrency()},
            {"availableSeats", f.getAvailableSeats()},
            {"source", f.getSource()}
        });
    }
    return json{
        {"provider", APIHandler::activeFlightProviderName()},
        {"bookable", APIHandler::flightResultsAreBookable()},
        {"flights", arr}
    };
}

} // namespace

int main() {
    crow::App<crow::CORSHandler> app;
    APIHandler::initializeAPIKeys();
    Logger::info("TravelPlanner API server starting up");

    // Permissive CORS for local demo purposes only - do not use this
    // configuration as-is in a production deployment.
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global().origin("*").methods("GET"_method, "POST"_method);

    // Serve the demo frontend at the site root.
    CROW_ROUTE(app, "/")
    ([]() {
        crow::response res;
        std::ifstream file("web/index.html");
        if (!file) {
            res.code = 404;
            res.body = "Demo frontend not found (web/index.html missing)";
            return res;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        res.set_header("Content-Type", "text/html");
        res.body = buffer.str();
        return res;
    });

    CROW_ROUTE(app, "/weather").methods("GET"_method)
    ([](const crow::request& req) {
        auto city = req.url_params.get("city");
        auto days_str = req.url_params.get("days");
        if (!city || !days_str) {
            return crow::response(400, "Missing city or days parameter");
        }
        int days = std::stoi(days_str);
        try {
            json result = APIHandler::getWeatherJson(city, days);
            return crow::response(result.dump());
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    CROW_ROUTE(app, "/flights").methods("GET"_method)
    ([](const crow::request& req) {
        auto from = req.url_params.get("from");
        auto to = req.url_params.get("to");
        auto date = req.url_params.get("date");
        auto passengers_str = req.url_params.get("passengers");
        if (!from || !to || !date || !passengers_str) {
            return crow::response(400, "Missing required parameters");
        }
        int passengers = std::stoi(passengers_str);
        try {
            auto flights = APIHandler::searchFlights(from, to, date, passengers);
            return crow::response(serializeFlights(flights).dump());
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    CROW_ROUTE(app, "/hotels").methods("GET"_method)
    ([](const crow::request& req) {
        auto city = req.url_params.get("city");
        auto checkin = req.url_params.get("checkin");
        auto checkout = req.url_params.get("checkout");
        auto guests_str = req.url_params.get("guests");
        if (!city || !checkin || !checkout || !guests_str) {
            return crow::response(400, "Missing required parameters");
        }
        int guests = std::stoi(guests_str);
        try {
            auto hotels = APIHandler::searchHotels(city, checkin, checkout, guests);
            json arr = json::array();
            for (const auto& h : hotels) {
                arr.push_back({
                    {"name", h.getName()},
                    {"location", h.getLocation()},
                    {"pricePerNight", h.getPricePerNight()},
                    {"rating", h.getRating()},
                    {"address", h.getAddress()}
                });
            }
            return crow::response(arr.dump());
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    CROW_ROUTE(app, "/itinerary").methods("GET"_method)
    ([](const crow::request& req) {
        auto destination = req.url_params.get("destination");
        auto start = req.url_params.get("start");
        auto end = req.url_params.get("end");
        auto people_str = req.url_params.get("people");
        auto budget_str = req.url_params.get("budget");
        auto hotel = req.url_params.get("hotel");
        if (!destination || !start || !end || !people_str || !budget_str || !hotel) {
            return crow::response(400, "Missing required parameters");
        }
        int people = std::stoi(people_str);
        double budget = std::stod(budget_str);
        // For demo, create a dummy hotel (in real use, parse hotel JSON or fetch from DB)
        Hotel selectedHotel(hotel, destination, 0, 0, start, end, "", APIHandler::CURRENCY_CODE);
        try {
            auto items = APIHandler::generateItinerary(destination, start, end, people, budget, selectedHotel);
            json arr = json::array();
            for (const auto& item : items) {
                arr.push_back({
                    {"activity", item.getActivity()},
                    {"date", item.getDate()},
                    {"time", item.getTime()},
                    {"category", item.getCategory()}
                });
            }
            return crow::response(arr.dump());
        } catch (const std::exception& e) {

            return crow::response(500, e.what());
        }
    });
    // Add POST endpoint for /flights (search flights with JSON body)
    CROW_ROUTE(app, "/flights").methods("POST"_method)
    ([](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            auto from = body.value("from", "");
            auto to = body.value("to", "");
            auto date = body.value("date", "");
            int passengers = body.value("passengers", 1);
            if (from.empty() || to.empty() || date.empty()) {
                return crow::response(400, "Missing required parameters in JSON body");
            }
            auto flights = APIHandler::searchFlights(from, to, date, passengers);
            return crow::response(serializeFlights(flights).dump());
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    // Add POST endpoint for /hotels (search hotels with JSON body)
    CROW_ROUTE(app, "/hotels").methods("POST"_method)
    ([](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            auto city = body.value("city", "");
            auto checkin = body.value("checkin", "");
            auto checkout = body.value("checkout", "");
            int guests = body.value("guests", 1);
            if (city.empty() || checkin.empty() || checkout.empty()) {
                return crow::response(400, "Missing required parameters in JSON body");
            }
            auto hotels = APIHandler::searchHotels(city, checkin, checkout, guests);
            json arr = json::array();
            for (const auto& h : hotels) {
                arr.push_back({
                    {"name", h.getName()},
                    {"location", h.getLocation()},
                    {"pricePerNight", h.getPricePerNight()},
                    {"rating", h.getRating()},
                    {"address", h.getAddress()}
                });
            }
            return crow::response(arr.dump());
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    // Add POST endpoint for /itinerary (generate itinerary with JSON body)
    CROW_ROUTE(app, "/itinerary").methods("POST"_method)
    ([](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            auto destination = body.value("destination", "");
            auto start = body.value("start", "");
            auto end = body.value("end", "");
            int people = body.value("people", 1);
            double budget = body.value("budget", 0.0);
            auto hotelName = body.value("hotel", "");
            if (destination.empty() || start.empty() || end.empty() || hotelName.empty()) {
                return crow::response(400, "Missing required parameters in JSON body");
            }
            Hotel selectedHotel(hotelName, destination, 0, 0, start, end, "", APIHandler::CURRENCY_CODE);
            auto items = APIHandler::generateItinerary(destination, start, end, people, budget, selectedHotel);
            json arr = json::array();
            for (const auto& item : items) {
                arr.push_back({
                    {"activity", item.getActivity()},
                    {"date", item.getDate()},
                    {"time", item.getTime()},
                    {"category", item.getCategory()}
                });
            }
            return crow::response(arr.dump());
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    int port = 8080;
    if (const char* portEnv = getenv("PORT")) {
        try { port = stoi(portEnv); } catch (...) {}
    }
    Logger::info("Listening on http://localhost:" + to_string(port));
    app.port(port).multithreaded().run();
    return 0;
}
