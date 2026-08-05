#pragma once
#include <string>
#include <vector>
#include "itinerary_item.hpp"
using namespace std;

class Trip {
private:
    string destination;
    string startDate;
    string endDate;
    int peopleCount;
    double budget;
    string currency;
    vector<ItineraryItem> itinerary;

public:
    //This function constructs a new Trip object with the given destination, start date, end date, number of people, budget, and currency
    Trip(string dest, string start, string end, int count, double budget = 0.0, string currency = "INR");

    //This function displays the trip details
    void displayTrip() const;

    //This function adds an item to the itinerary
    void addItineraryItem(const ItineraryItem& item);

    //This function displays the complete itinerary
    void displayItinerary() const;

    string getDestination() const;
    string getStartDate() const;
    string getEndDate() const;
    int getPeopleCount() const;
    double getBudget() const;
    string getCurrency() const { return currency; }
    const vector<ItineraryItem>& getItinerary() const { return itinerary; }
};