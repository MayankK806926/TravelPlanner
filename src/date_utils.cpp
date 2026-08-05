#include "date_utils.hpp"

namespace DateUtils {

namespace {
bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && isLeapYear(year)) return 29;
    return days[month - 1];
}
}

bool isValidDate(int year, int month, int day, int minYear) {
    if (year < minYear) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > daysInMonth(year, month)) return false;
    return true;
}

bool isValidDateString(const std::string& date, int minYear) {
    if (date.length() != 10 || date[4] != '-' || date[7] != '-') return false;

    for (int i : {0, 1, 2, 3, 5, 6, 8, 9}) {
        if (!isdigit(static_cast<unsigned char>(date[i]))) return false;
    }

    try {
        int year = std::stoi(date.substr(0, 4));
        int month = std::stoi(date.substr(5, 2));
        int day = std::stoi(date.substr(8, 2));
        return isValidDate(year, month, day, minYear);
    } catch (...) {
        return false;
    }
}

} // namespace DateUtils
