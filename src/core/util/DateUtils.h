#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include <string>

// Small, Qt-free date/time helpers shared by seasons, bills and reports.
namespace DateUtils
{
    // True for a real calendar date in YYYY-MM-DD form.
    bool isValidDate(const std::string& date);

    // True for HH:MM:SS (00:00:00 - 23:59:59).
    bool isValidTime(const std::string& time);

    // Local calendar date / clock time, e.g. 2026-11-21 and 14:05:09.
    std::string today();
    std::string nowTime();

    // Lexicographic comparison is valid for YYYY-MM-DD. Both inputs must be
    // valid dates.
    int compareDates(const std::string& left, const std::string& right);
}

#endif // DATE_UTILS_H
