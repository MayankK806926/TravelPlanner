#include "trip.hpp"
#include <iostream>
using namespace std;

//This function constructs a new Trip object with the given destination, start date, end date, number of people, and budget
Trip::Trip(string dest, string start, string end, int count, double budget)
    : destination(dest), startDate(start), endDate(end), peopleCount(count), budget(budget) {}

//This function displays the trip details
void Trip::displayTrip() const {
    cout << "\nTrip Details:" << endl;
    cout << "Destination: " << destination << endl;
    cout << "Dates: " << startDate << " to " << endDate << endl;
    cout << "People: " << peopleCount << endl;
    if (budget > 0) {
        cout << "Budget: " << budget << " INR" << endl;
    }
}

//This function adds an item to the itinerary
void Trip::addItineraryItem(const ItineraryItem& item) {
    itinerary.push_back(item);
}

//This function displays the complete itinerary
void Trip::displayItinerary() const {
    cout << "\nItinerary for " << destination << ":" << endl;
    for (const auto& item : itinerary) {
        item.displayDetails();
    }
}

//These functions return the destination, start date, end date, number of people, and budget
string Trip::getDestination() const { return destination; }
string Trip::getStartDate() const { return startDate; }
string Trip::getEndDate() const { return endDate; }
int Trip::getPeopleCount() const { return peopleCount; }
double Trip::getBudget() const { return budget; }