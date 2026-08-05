#pragma once
#include <string>
using namespace std;

class Hotel {
private:
    string name;
    string location;
    double pricePerNight;
    double rating;
    string checkInDate;
    string checkOutDate;
    string address;
    string currency;

public:
    /**
     * @brief Construct a new Hotel object
     * @param n Hotel name
     * @param loc Location/address
     * @param price Price per night
     * @param rat Rating (0-5)
     * @param checkIn Check-in date
     * @param checkOut Check-out date
     * @param addr Address
     * @param currency ISO currency code for price (default "INR")
     */
    Hotel(string n, string loc, double price, double rat,
          string checkIn, string checkOut, string addr = "", string currency = "INR");

    /**
     * @brief Display hotel information
     */
    void displayInfo() const;

    // Getters
    string getName() const;
    string getLocation() const;
    double getPricePerNight() const;
    double getRating() const;
    string getCheckInDate() const;
    string getCheckOutDate() const;
    string getAddress() const;
};
