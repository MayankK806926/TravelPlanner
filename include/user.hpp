#pragma once
#include <string>
#include "trip.hpp"
using namespace std;

class User {
private:
    string username;
    string email;
    string password;
    Trip* currentTrip;  // Using raw pointer since Trip lifetime is managed by User

public:
    User();  // Constructor
    ~User(); // Destructor to clean up Trip

    //This function registers a new user with username, email, and password
    void registerUser();

    //This function displays the user's profile information
    void displayProfile() const;

    //This function sets the current trip being planned
    void setTrip(Trip* trip);

    //This function returns the current trip being planned
    Trip* getTrip() const;
};