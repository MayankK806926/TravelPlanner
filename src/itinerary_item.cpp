#include "itinerary_item.hpp"
#include <iostream>
using namespace std;

//This function constructs a new ItineraryItem object with the given activity, date, time, and category
ItineraryItem::ItineraryItem(string act, string d, string t, string cat)
    : activity(act), date(d), time(t), category(cat) {}

//This function displays the itinerary item details
void ItineraryItem::displayDetails() const {
    cout << "Date: " << date << endl
         << "Time: " << time << endl
         << "Activity: " << activity << endl
         << "Category: " << category << endl
         << "-------------------" << endl;
}

//These functions return the activity, date, time, and category
string ItineraryItem::getActivity() const { return activity; }
string ItineraryItem::getDate() const { return date; }
string ItineraryItem::getTime() const { return time; }
string ItineraryItem::getCategory() const { return category; }