#include <iostream>
#include <string>
#include <limits>
#include <iomanip>
#include <curl/curl.h>
#include "user.hpp"
#include "trip.hpp"
#include "api_handler.hpp"
#include "itinerary_item.hpp"
#include "hotel.hpp"
#include "flight.hpp"
#include <algorithm>

using namespace std;
using std::min;
using std::max;

// Function to get valid date input with validation
string getDateInput(const string& prompt) {
    while (true) {
        string date;
        cout << prompt;
        getline(cin, date);
        
        if (date.length() == 10 && date[4] == '-' && date[7] == '-') {
            try {
                int year = stoi(date.substr(0, 4));
                int month = stoi(date.substr(5, 2));
                int day = stoi(date.substr(8, 2));
                
                if (year >= 2024 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
                    return date;
                }
            } catch (...) {}
        }
        cout << "Invalid date format. Please use YYYY-MM-DD format." << endl;
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

int main() {
    try {
        if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
            throw runtime_error("Failed to initialize curl");
        }
        
        cout << "Welcome to the Travel Planner!" << endl;
        // cout << "Initializing API keys..." << endl;
        APIHandler::initializeAPIKeys();
        
        User currentUser;
        currentUser.registerUser();
        currentUser.displayProfile();

        double totalCost = 0.0;
        string destination;
        string startDate, endDate;
        int peopleCount;
        Flight* selectedOutboundFlight = nullptr;
        Flight* selectedReturnFlight = nullptr;
        Hotel* selectedHotel = nullptr;
        bool isDestinationSelected = false;
        vector<Flight> selectedOutboundLegs;
        vector<Flight> selectedReturnLegs;
        Trip* trip = nullptr;

        while (!isDestinationSelected) {
            cout << "\nEnter destination city: ";
            getline(cin, destination);

            // Weather forecast
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

        // Outbound flights
        vector<Flight> outboundFlights;
        try {
            cout << "\nSearching for outbound flights..." << endl;
            cout << "From: " << boardingCity << " To: " << destination << endl;
            outboundFlights = APIHandler::searchFlights(boardingCity, destination, startDate, peopleCount);
        } catch (const std::exception& e) {
            cout << "\n[Warning] Outbound flight search failed: " << e.what() << endl;
        }
        vector<FlightJourney> outboundJourneys;
        if (!outboundFlights.empty()) {
            // Group flights into journeys (connecting flights)
            for (size_t i = 0; i < outboundFlights.size();) {
                FlightJourney journey;
                journey.legs.push_back(outboundFlights[i]);
                journey.totalPrice = outboundFlights[i].getPrice();
                journey.minSeats = outboundFlights[i].getAvailableSeats();
                size_t j = i;
                // Group connecting legs
                while (j + 1 < outboundFlights.size() &&
                       outboundFlights[j].getArrivalAirport() == outboundFlights[j + 1].getDepartureAirport()) {
                    ++j;
                    journey.legs.push_back(outboundFlights[j]);
                    journey.totalPrice += outboundFlights[j].getPrice();
                    journey.minSeats = min(journey.minSeats, outboundFlights[j].getAvailableSeats());
                }
                outboundJourneys.push_back(journey);
                i = j + 1;
            }
            size_t numOptions = min(size_t(5), outboundJourneys.size());
            cout << "\nAvailable Flight Options (Connecting flights grouped):" << endl;
            for (size_t i = 0; i < numOptions; ++i) {
                cout << "\nOption " << (i + 1) << ":" << endl;
                for (size_t leg = 0; leg < outboundJourneys[i].legs.size(); ++leg) {
                    cout << "  Leg " << (leg + 1) << ": ";
                    outboundJourneys[i].legs[leg].displayInfo();
                    outboundJourneys[i].legs[leg].displayPrice();
                }
                cout << "Total Journey Price: " << fixed << setprecision(2) << outboundJourneys[i].totalPrice << " INR" << endl;
                cout << "Minimum Available Seats: " << outboundJourneys[i].minSeats << endl;
                cout << string(50, '-') << endl;
            }
            // Select outbound journey
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
                cout << "Total Journey Price: " << fixed << setprecision(2) << outboundJourneys[choice - 1].totalPrice << " INR" << endl;
                cout << "Current total cost: " << totalCost << " INR" << endl;
                if (getYesNoInput("Confirm this outbound journey?")) break;
                totalCost -= outboundJourneys[choice - 1].totalPrice * peopleCount;
                selectedOutboundLegs.clear();
            }
        } else {
            cout << "\nNo outbound flights found or failed to fetch. Continuing to next step..." << endl;
        }

        // Return flights
        vector<Flight> returnFlights;
        size_t numOptions = 0; // Declare numOptions at the right scope
        try {
            cout << "\nSearching for return flights..." << endl;
            cout << "From: " << destination << " To: " << boardingCity << endl;
            returnFlights = APIHandler::searchFlights(destination, boardingCity, endDate, peopleCount);
        } catch (const std::exception& e) {
            cout << "\n[Warning] Return flight search failed: " << e.what() << endl;
        }
        vector<FlightJourney> returnJourneys;
        if (!returnFlights.empty()) {
            // Group return flights into journeys
            for (size_t i = 0; i < returnFlights.size();) {
                FlightJourney journey;
                journey.legs.push_back(returnFlights[i]);
                journey.totalPrice = returnFlights[i].getPrice();
                journey.minSeats = returnFlights[i].getAvailableSeats();
                size_t j = i;
                while (j + 1 < returnFlights.size() &&
                       returnFlights[j].getArrivalAirport() == returnFlights[j + 1].getDepartureAirport()) {
                    ++j;
                    journey.legs.push_back(returnFlights[j]);
                    journey.totalPrice += returnFlights[j].getPrice();
                    journey.minSeats = min(journey.minSeats, returnFlights[j].getAvailableSeats());
                }
                returnJourneys.push_back(journey);
                i = j + 1;
            }
            numOptions = min(size_t(5), returnJourneys.size());
            cout << "\nAvailable Return Flight Options (Connecting flights grouped):" << endl;
            for (size_t i = 0; i < numOptions; ++i) {
                cout << "\nOption " << (i + 1) << ":" << endl;
                for (size_t leg = 0; leg < returnJourneys[i].legs.size(); ++leg) {
                    cout << "  Leg " << (leg + 1) << ": ";
                    returnJourneys[i].legs[leg].displayInfo();
                    returnJourneys[i].legs[leg].displayPrice();
                }
                cout << "Total Journey Price: " << fixed << setprecision(2) << returnJourneys[i].totalPrice << " INR" << endl;
                cout << "Minimum Available Seats: " << returnJourneys[i].minSeats << endl;
                cout << string(50, '-') << endl;
            }
            // Select return journey
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
                cout << "Total Journey Price: " << fixed << setprecision(2) << returnJourneys[choice - 1].totalPrice << " INR" << endl;
                cout << "Current total cost: " << totalCost << " INR" << endl;
                if (getYesNoInput("Confirm this return journey?")) break;
                totalCost -= returnJourneys[choice - 1].totalPrice * peopleCount;
                selectedReturnLegs.clear();
            }
        } else {
            cout << "\nNo return flights found or failed to fetch. Continuing to next step..." << endl;
        }

        // Hotels
        vector<Hotel> hotels;
        try {
            cout << "\nSearching for available hotels..." << endl;
            hotels = APIHandler::searchHotels(destination, startDate, endDate, peopleCount);
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
                selectedHotel = new Hotel(hotels[choice - 1]);
                totalCost += selectedHotel->getPricePerNight() * peopleCount;
                
                cout << "\nCurrent total cost: " << totalCost << " INR" << endl;
                if (getYesNoInput("Confirm this hotel?")) break;
                totalCost -= selectedHotel->getPricePerNight() * peopleCount;
                delete selectedHotel;
                selectedHotel = nullptr;
            }
        } else {
            cout << "\nNo hotels found or failed to fetch. Continuing to next step..." << endl;
        }

        // Itinerary
        vector<ItineraryItem> generatedItinerary;
        try {
            cout << "\nGenerating personalized itinerary..." << endl;
            if (!hotels.empty() && selectedHotel) {
                generatedItinerary = APIHandler::generateItinerary(destination, startDate, endDate, peopleCount, totalCost, *selectedHotel);
            } else {
                // Use a dummy hotel if none selected
                Hotel dummyHotel("Hotel", destination, 0, 0, startDate, endDate, "");
                generatedItinerary = APIHandler::generateItinerary(destination, startDate, endDate, peopleCount, totalCost, dummyHotel);
            }
        } catch (const std::exception& e) {
            cout << "\n[Warning] Itinerary generation failed: " << e.what() << endl;
        }
        // Ensure trip is created before adding itinerary or printing plan
        if (!trip) {
            trip = new Trip(destination, startDate, endDate, peopleCount, totalCost);
            currentUser.setTrip(trip);
        }
        if (!generatedItinerary.empty()) {
            for (const auto& item : generatedItinerary) {
                trip->addItineraryItem(item);
            }
        } else {
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
        cout << "\nTotal Trip Cost: " << fixed << setprecision(2) << totalCost << " INR" << endl;

        cout << "\nThank you for using our service! Happy Journey!" << endl;

        delete selectedOutboundFlight;
        delete selectedReturnFlight;
        delete selectedHotel;
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