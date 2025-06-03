#include "hotel.hpp"
#include <iostream>
#include <iomanip> // for setprecision
using namespace std;

Hotel::Hotel(string n, string loc, double price, double rat, 
             string checkIn, string checkOut, string addr)
    : name(n), location(loc), pricePerNight(price), rating(rat),
      checkInDate(checkIn), checkOutDate(checkOut), address(addr) {}

void Hotel::displayInfo() const {
    cout << "Hotel: " << name << endl;
    cout << "Rating: " << rating << " stars" << endl;
    cout << "Total Stay Cost: " << fixed << setprecision(2) << pricePerNight << " INR" << endl;
    cout << "Address: " << address << endl;
}

// Getters implementation
string Hotel::getName() const { return name; }
string Hotel::getLocation() const { return location; }
double Hotel::getPricePerNight() const { return pricePerNight; }
double Hotel::getRating() const { return rating; }
string Hotel::getCheckInDate() const { return checkInDate; }
string Hotel::getCheckOutDate() const { return checkOutDate; }
string Hotel::getAddress() const { return address; }