#include "user.hpp"
#include <iostream>
using namespace std;

void User::registerUser() {
    cout << "\nUser Registration\n" << endl;
    cout << "Enter username: ";
    getline(cin, username);
    cout << "Enter email: ";
    getline(cin, email);
}

void User::displayProfile() const {
    cout << "\nUser Profile:" << endl;
    cout << "Username: " << username << endl;
    cout << "Email: " << email << endl;
}

void User::setTrip(unique_ptr<Trip> trip) {
    currentTrip = std::move(trip);
}

Trip* User::getTrip() const {
    return currentTrip.get();
}
