#include "user.hpp"
#include <iostream>
using namespace std;

User::User() : currentTrip(nullptr) {}

User::~User() {
    delete currentTrip;
}

void User::registerUser() {
    cout << "\nUser Registration\n" << endl;
    cout << "Enter username: ";
    getline(cin, username);
    cout << "Enter email: ";
    getline(cin, email);
    cout << "Enter password: ";
    getline(cin, password);
}

void User::displayProfile() const {
    cout << "\nUser Profile:" << endl;
    cout << "Username: " << username << endl;
    cout << "Email: " << email << endl;
}

void User::setTrip(Trip* trip) {
    delete currentTrip;  // Delete any existing trip
    currentTrip = trip;
}

Trip* User::getTrip() const {
    return currentTrip;
}