#pragma once
#include <string>
#include <memory>
#include "trip.hpp"
using namespace std;

class User {
private:
    string username;
    string email;
    unique_ptr<Trip> currentTrip;

public:
    User() = default;

    //This function registers a new user with username and email
    void registerUser();

    //This function displays the user's profile information
    void displayProfile() const;

    //This function sets the current trip being planned (User takes ownership)
    void setTrip(unique_ptr<Trip> trip);

    //This function returns the current trip being planned
    Trip* getTrip() const;

    string getUsername() const { return username; }
    string getEmail() const { return email; }
};
