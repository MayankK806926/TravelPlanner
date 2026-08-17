#pragma once
#include <string>
#include <ctime>
#include <vector>
using namespace std;

class Flight {
private:
    string airline;
    string flightNumber;
    string departureAirport;
    string arrivalAirport;
    tm departureTime;
    tm arrivalTime;
    double price;
    int availableSeats;
    string currency;
    string source;  // "amadeus" = live inventory, "estimate"/"mock" = not bookable

public:
    /**
     * @brief Construct a new Flight object
     * @param airline Airline name
     * @param flightNum Flight number
     * @param depAirport Departure airport code
     * @param arrAirport Arrival airport code
     * @param depTime Departure time
     * @param arrTime Arrival time
     * @param price Ticket price
     * @param seats Available seats
     * @param currency ISO currency code for price (default "INR")
     * @param source Origin of the data: "amadeus" for live bookable
     *        inventory, "estimate" or "mock" for representative data
     */
    Flight(string airline, string flightNum, string depAirport,
           string arrAirport, tm depTime, tm arrTime,
           double price, int seats, string currency = "INR",
           string source = "amadeus");

    /**
     * @brief Display flight information
     */
    void displayInfo() const;

    /**
     * @brief Display flight price
     */
    void displayPrice() const;

    // Getters
    string getAirline() const;
    string getFlightNumber() const;
    string getDepartureAirport() const;
    string getArrivalAirport() const;
    tm getDepartureTime() const;
    tm getArrivalTime() const;
    double getPrice() const;
    int getAvailableSeats() const;
    string getCurrency() const;
    string getSource() const;

    // False when the data is an estimate rather than bookable inventory.
    bool isBookable() const;
};
