#ifndef DATE_UTILS_HPP
#define DATE_UTILS_HPP

#include <string>

// Pure, side-effect-free date helpers - kept out of any cin/cout code so
// they're straightforward to unit test.
namespace DateUtils {

// True if year/month/day form a real calendar date (accounts for leap
// years and per-month day counts), year must be >= minYear.
bool isValidDate(int year, int month, int day, int minYear = 2024);

// True if `date` is exactly "YYYY-MM-DD" and represents a real calendar
// date >= minYear.
bool isValidDateString(const std::string& date, int minYear = 2024);

} // namespace DateUtils

#endif // DATE_UTILS_HPP
