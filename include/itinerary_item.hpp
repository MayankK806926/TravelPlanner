#ifndef ITINERARY_ITEM_HPP
#define ITINERARY_ITEM_HPP

#include <string>
using namespace std;

class ItineraryItem {
private:  // Changed to private for better encapsulation
    string activity;
    string date;
    string time;
    string category; // e.g., Landmark, Food, Shopping, etc.

public:
    // Default constructor
    ItineraryItem() = default;

    // Constructor with parameters
    ItineraryItem(string act, string d, string t, string cat);

    // Display function
    void displayDetails() const;

    // Getters
    string getActivity() const;
    string getDate() const;
    string getTime() const;
    string getCategory() const;
};

#endif // ITINERARY_ITEM_HPP