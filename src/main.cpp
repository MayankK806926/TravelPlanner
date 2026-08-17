#include <iostream>
#include <string>
#include <limits>
#include <iomanip>
#include <curl/curl.h>
#include <future>
#include "user.hpp"
#include "trip.hpp"
#include "api_handler.hpp"
#include "itinerary_item.hpp"
#include "hotel.hpp"
#include "flight.hpp"
#include "date_utils.hpp"
#include "logger.hpp"
#include <algorithm>
#ifdef HAVE_PERSISTENCE
#include "trip_repository.hpp"
#endif

using namespace std;
using std::min;
using std::max;

// Function to get valid date input with real calendar validation
string getDateInput(const string& prompt) {
    while (true) {
        string date;
        cout << prompt;
        getline(cin, date);

        if (DateUtils::isValidDateString(date)) {
            return date;
        }
        cout << "Invalid date. Please use YYYY-MM-DD format with a real calendar date." << endl;
    }
}

// Function to clear input buffer and handle invalid input
void clearInputBuffer() {
    cin.clear();
    cin.ignore(10000, '\n'); // Use a large number to clear the buffer
}

// Function to get integer input with validation
int getIntegerInput(const string& prompt, int min, int max) {
    while (true) {
        cout << prompt;
        int value;
        if (cin >> value && value >= min && value <= max) {
            clearInputBuffer();
            return value;
        }
        cout << "Invalid input. Please enter a number between " << min << " and " << max << endl;
        clearInputBuffer();
    }
}

// Function to get yes/no input
bool getYesNoInput(const string& prompt) {
    while (true) {
        string response;
        cout << prompt << " (y/n): ";
        getline(cin, response);
        if (response == "y" || response == "Y") return true;
        if (response == "n" || response == "N") return false;
        cout << "Please enter 'y' or 'n'" << endl;
    }
}

// Struct for grouping flight journeys
struct FlightJourney {
    vector<Flight> legs;
    double totalPrice;
    int minSeats;
};

// Groups a flat list of connecting-flight legs into journeys.
vector<FlightJourney> groupIntoJourneys(const vector<Flight>& flights) {
    vector<FlightJourney> journeys;
    for (size_t i = 0; i < flights.size();) {
        FlightJourney journey;
        journey.legs.push_back(flights[i]);
        journey.totalPrice = flights[i].getPrice();
        journey.minSeats = flights[i].getAvailableSeats();
        size_t j = i;
        while (j + 1 < flights.size() &&
               flights[j].getArrivalAirport() == flights[j + 1].getDepartureAirport()) {
            ++j;
            journey.legs.push_back(flights[j]);
            journey.totalPrice += flights[j].getPrice();
            journey.minSeats = min(journey.minSeats, flights[j].getAvailableSeats());
        }
        journeys.push_back(journey);
        i = j + 1;
    }
    return journeys;
}

int main() {
    try {
        if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
            throw runtime_error("Failed to initialize curl");
        }

        cout << "Welcome to the Travel Planner!" << endl;
        APIHandler::initializeAPIKeys();

        User currentUser;
        currentUser.registerUser();
        currentUser.displayProfile();

        double totalCost = 0.0;
        string destination;
        string startDate, endDate;
        int peopleCount;
        vector<Flight> selectedOutboundLegs;
        vector<Flight> selectedReturnLegs;
        unique_ptr<Hotel> selectedHotel;

        bool isDestinationSelected = false;
        while (!isDestinationSelected) {
            cout << "\nEnter destination city: ";
            getline(cin, destination);

            try {
                int forecastDays = getIntegerInput("\nHow many days of weather forecast would you like to see? (1-14): ", 1, 14);
                cout << "\nFetching weather forecast..." << endl;
                APIHandler::getWeather(destination, forecastDays);
            } catch (const std::exception& e) {
                cout << "\n[Warning] Weather forecast unavailable: " << e.what() << endl;
            }

            if (!getYesNoInput("\nAre you satisfied with the weather conditions?")) {
                cout << "Let's try a different destination." << endl;
                continue;
            }

            startDate = getDateInput("\nEnter start date (YYYY-MM-DD): ");
            endDate = getDateInput("Enter end date (YYYY-MM-DD): ");

            if (getYesNoInput("\nConfirm these travel dates?")) {
                isDestinationSelected = true;
            }
        }

        peopleCount = getIntegerInput("Enter number of people (1-10): ", 1, 10);

        string boardingCity;
        cout << "\nEnter your boarding city: ";
        getline(cin, boardingCity);

        // Outbound flights, return flights, and hotels are independent
        // lookups once destination/dates/boarding city are known - fire
        // them off concurrently instead of waiting on each in turn.
        if (!APIHandler::flightResultsAreBookable()) {
            cout << "\n[Notice] Flight results come from the '"
                 << APIHandler::activeFlightProviderName()
                 << "' provider and are ESTIMATES for planning only - they are not\n"
                    "         real bookable flights. Configure a flight data provider for live inventory."
                 << endl;
        }

        cout << "\nSearching for outbound flights, return flights, and hotels..." << endl;
        auto outboundFuture = std::async(std::launch::async, [&]() {
            return APIHandler::searchFlights(boardingCity, destination, startDate, peopleCount);
        });
        auto returnFuture = std::async(std::launch::async, [&]() {
            return APIHandler::searchFlights(destination, boardingCity, endDate, peopleCount);
        });
        auto hotelsFuture = std::async(std::launch::async, [&]() {
            return APIHandler::searchHotels(destination, startDate, endDate, peopleCount);
        });

        vector<Flight> outboundFlights;
        try {
            outboundFlights = outboundFuture.get();
        } catch (const std::exception& e) {
            cout << "\n[Warning] Outbound flight search failed: " << e.what() << endl;
        }
        vector<FlightJourney> outboundJourneys = groupIntoJourneys(outboundFlights);
        if (!outboundJourneys.empty()) {
            size_t numOptions = min(size_t(5), outboundJourneys.size());
            cout << "\nAvailable Flight Options (Connecting flights grouped):" << endl;
            for (size_t i = 0; i < numOptions; ++i) {
                cout << "\nOption " << (i + 1) << ":" << endl;
                for (size_t leg = 0; leg < outboundJourneys[i].legs.size(); ++leg) {
                    cout << "  Leg " << (leg + 1) << ": ";
                    outboundJourneys[i].legs[leg].displayInfo();
                    outboundJourneys[i].legs[leg].displayPrice();
                }
                cout << "Total Journey Price: " << fixed << setprecision(2) << outboundJourneys[i].totalPrice << " " << APIHandler::CURRENCY_CODE << endl;
                cout << "Minimum Available Seats: " << outboundJourneys[i].minSeats << endl;
                cout << string(50, '-') << endl;
            }
            while (true) {
                int choice = getIntegerInput("\nSelect an outbound flight (1-" + to_string(numOptions) + "): ", 1, numOptions);
                selectedOutboundLegs = outboundJourneys[choice - 1].legs;
                totalCost += outboundJourneys[choice - 1].totalPrice * peopleCount;
                cout << "\nSelected outbound journey details:" << endl;
                for (size_t leg = 0; leg < selectedOutboundLegs.size(); ++leg) {
                    cout << "  Leg " << (leg + 1) << ": ";
                    selectedOutboundLegs[leg].displayInfo();
                    selectedOutboundLegs[leg].displayPrice();
                }
                cout << "Total Journey Price: " << fixed << setprecision(2) << outboundJourneys[choice - 1].totalPrice << " " << APIHandler::CURRENCY_CODE << endl;
                cout << "Current total cost: " << totalCost << " " << APIHandler::CURRENCY_CODE << endl;
                if (getYesNoInput("Confirm this outbound journey?")) break;
                totalCost -= outboundJourneys[choice - 1].totalPrice * peopleCount;
                selectedOutboundLegs.clear();
            }
        } else {
            cout << "\nNo outbound flights found or failed to fetch. Continuing to next step..." << endl;
        }

        vector<Flight> returnFlights;
        try {
            returnFlights = returnFuture.get();
        } catch (const std::exception& e) {
            cout << "\n[Warning] Return flight search failed: " << e.what() << endl;
        }
        vector<FlightJourney> returnJourneys = groupIntoJourneys(returnFlights);
        if (!returnJourneys.empty()) {
            size_t numOptions = min(size_t(5), returnJourneys.size());
            cout << "\nAvailable Return Flight Options (Connecting flights grouped):" << endl;
            for (size_t i = 0; i < numOptions; ++i) {
                cout << "\nOption " << (i + 1) << ":" << endl;
                for (size_t leg = 0; leg < returnJourneys[i].legs.size(); ++leg) {
                    cout << "  Leg " << (leg + 1) << ": ";
                    returnJourneys[i].legs[leg].displayInfo();
                    returnJourneys[i].legs[leg].displayPrice();
                }
                cout << "Total Journey Price: " << fixed << setprecision(2) << returnJourneys[i].totalPrice << " " << APIHandler::CURRENCY_CODE << endl;
                cout << "Minimum Available Seats: " << returnJourneys[i].minSeats << endl;
                cout << string(50, '-') << endl;
            }
            while (true) {
                int choice = getIntegerInput("\nSelect a return flight (1-" + to_string(numOptions) + "): ", 1, numOptions);
                selectedReturnLegs = returnJourneys[choice - 1].legs;
                totalCost += returnJourneys[choice - 1].totalPrice * peopleCount;
                cout << "\nSelected return journey details:" << endl;
                for (size_t leg = 0; leg < selectedReturnLegs.size(); ++leg) {
                    cout << "  Leg " << (leg + 1) << ": ";
                    selectedReturnLegs[leg].displayInfo();
                    selectedReturnLegs[leg].displayPrice();
                }
                cout << "Total Journey Price: " << fixed << setprecision(2) << returnJourneys[choice - 1].totalPrice << " " << APIHandler::CURRENCY_CODE << endl;
                cout << "Current total cost: " << totalCost << " " << APIHandler::CURRENCY_CODE << endl;
                if (getYesNoInput("Confirm this return journey?")) break;
                totalCost -= returnJourneys[choice - 1].totalPrice * peopleCount;
                selectedReturnLegs.clear();
            }
        } else {
            cout << "\nNo return flights found or failed to fetch. Continuing to next step..." << endl;
        }

        vector<Hotel> hotels;
        try {
            hotels = hotelsFuture.get();
        } catch (const std::exception& e) {
            cout << "\n[Warning] Hotel search failed: " << e.what() << endl;
        }
        if (!hotels.empty()) {
            cout << "\nAvailable Hotels:" << endl;
            for (size_t i = 0; i < hotels.size(); ++i) {
                cout << "\nOption " << (i + 1) << ":" << endl;
                hotels[i].displayInfo();
            }

            while (true) {
                int choice = getIntegerInput("\nSelect a hotel (1-" + to_string(hotels.size()) + "): ",
                                           1, hotels.size());
                selectedHotel = make_unique<Hotel>(hotels[choice - 1]);
                totalCost += selectedHotel->getPricePerNight() * peopleCount;

                cout << "\nCurrent total cost: " << totalCost << " " << APIHandler::CURRENCY_CODE << endl;
                if (getYesNoInput("Confirm this hotel?")) break;
                totalCost -= selectedHotel->getPricePerNight() * peopleCount;
                selectedHotel.reset();
            }
        } else {
            cout << "\nNo hotels found or failed to fetch. Continuing to next step..." << endl;
        }

        // Itinerary
        vector<ItineraryItem> generatedItinerary;
        try {
            cout << "\nGenerating personalized itinerary..." << endl;
            if (selectedHotel) {
                generatedItinerary = APIHandler::generateItinerary(destination, startDate, endDate, peopleCount, totalCost, *selectedHotel);
            } else {
                Hotel dummyHotel("Hotel", destination, 0, 0, startDate, endDate, "", APIHandler::CURRENCY_CODE);
                generatedItinerary = APIHandler::generateItinerary(destination, startDate, endDate, peopleCount, totalCost, dummyHotel);
            }
        } catch (const std::exception& e) {
            cout << "\n[Warning] Itinerary generation failed: " << e.what() << endl;
        }

        auto trip = make_unique<Trip>(destination, startDate, endDate, peopleCount, totalCost, APIHandler::CURRENCY_CODE);
        for (const auto& item : generatedItinerary) {
            trip->addItineraryItem(item);
        }
        if (generatedItinerary.empty()) {
            cout << "\nNo itinerary generated." << endl;
        }

        cout << "\n=== Your Complete Travel Plan ===" << endl;
        trip->displayTrip();
        cout << "\nSelected Outbound Journey:" << endl;
        for (size_t leg = 0; leg < selectedOutboundLegs.size(); ++leg) {
            cout << "  Leg " << (leg + 1) << ": ";
            selectedOutboundLegs[leg].displayInfo();
            selectedOutboundLegs[leg].displayPrice();
        }
        cout << "\nSelected Return Journey:" << endl;
        for (size_t leg = 0; leg < selectedReturnLegs.size(); ++leg) {
            cout << "  Leg " << (leg + 1) << ": ";
            selectedReturnLegs[leg].displayInfo();
            selectedReturnLegs[leg].displayPrice();
        }
        cout << "\nSelected Hotel:" << endl;
        if (selectedHotel) selectedHotel->displayInfo();
        else cout << "No hotel selected." << endl;
        cout << "\nDaily Itinerary:" << endl;
        trip->displayItinerary();
        cout << "\nTotal Trip Cost: " << fixed << setprecision(2) << totalCost << " " << APIHandler::CURRENCY_CODE << endl;

        cout << "\nThank you for using our service! Happy Journey!" << endl;

#ifdef HAVE_PERSISTENCE
        try {
            TripRepository repo;
            long long userId = repo.saveUser(currentUser.getUsername(), currentUser.getEmail());
            repo.saveTrip(userId, *trip);
        } catch (const std::exception& e) {
            cout << "\n[Warning] Could not save trip: " << e.what() << endl;
        }
#endif

        currentUser.setTrip(std::move(trip));
        curl_global_cleanup();
        return 0;
    } catch (const exception& e) {
        cout << "\nError: " << e.what() << endl;
        curl_global_cleanup();
        return 1;
    } catch (...) {
        cout << "\nAn unknown error occurred." << endl;
        curl_global_cleanup();
        return 1;
    }
}
