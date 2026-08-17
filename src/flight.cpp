#include "flight.hpp"
#include <iostream>
#include <iomanip>
#include <ctime>
using namespace std;

Flight::Flight(string airline, string flightNum, string depAirport,
               string arrAirport, tm depTime, tm arrTime,
               double price, int seats, string currency, string source)
    : airline(airline), flightNumber(flightNum), departureAirport(depAirport),
      arrivalAirport(arrAirport), departureTime(depTime), arrivalTime(arrTime),
      price(price), availableSeats(seats), currency(currency), source(source) {}

void Flight::displayInfo() const {
    char depTimeStr[80], arrTimeStr[80];
    strftime(depTimeStr, sizeof(depTimeStr), "%Y-%m-%d %H:%M", &departureTime);
    strftime(arrTimeStr, sizeof(arrTimeStr), "%Y-%m-%d %H:%M", &arrivalTime);

    cout << airline << " " << flightNumber;
    if (!isBookable()) {
        cout << "  [ESTIMATE - not a bookable flight]";
    }
    cout << endl;
    cout << departureAirport << " (" << depTimeStr << ") to "
         << arrivalAirport << " (" << arrTimeStr << ")" << endl;
}

void Flight::displayPrice() const {
    cout << "Price: " << fixed << setprecision(2) << price << " " << currency << endl;
    cout << "Available seats: " << availableSeats << endl;
}

// Getters implementation
string Flight::getAirline() const { return airline; }
string Flight::getFlightNumber() const { return flightNumber; }
string Flight::getDepartureAirport() const { return departureAirport; }
string Flight::getArrivalAirport() const { return arrivalAirport; }
tm Flight::getDepartureTime() const { return departureTime; }
tm Flight::getArrivalTime() const { return arrivalTime; }
double Flight::getPrice() const { return price; }
int Flight::getAvailableSeats() const { return availableSeats; }
string Flight::getCurrency() const { return currency; }
string Flight::getSource() const { return source; }
bool Flight::isBookable() const { return source == "amadeus"; }