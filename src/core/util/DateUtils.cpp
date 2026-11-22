#include "DateUtils.h"

#include <ctime>
#include <cstdio>
#include <stdexcept>

using std::string;

namespace
{
    bool isLeapYear(int year)
    {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    int daysInMonth(int year, int month)
    {
        static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (month == 2 && isLeapYear(year)) return 29;
        return days[month - 1];
    }

    string twoDigits(int value)
    {
        char buffer[3];
        std::snprintf(buffer, sizeof(buffer), "%02d", value);
        return string(buffer);
    }
}

namespace DateUtils
{
    bool isValidDate(const string& date)
    {
        if (date.size() != 10 || date[4] != '-' || date[7] != '-')
            return false;

        for (size_t i = 0; i < date.size(); i++)
        {
            if (i == 4 || i == 7) continue;
            if (date[i] < '0' || date[i] > '9') return false;
        }

        const int year = (date[0] - '0') * 1000 + (date[1] - '0') * 100 +
                         (date[2] - '0') * 10 + (date[3] - '0');
        const int month = (date[5] - '0') * 10 + (date[6] - '0');
        const int day = (date[8] - '0') * 10 + (date[9] - '0');

        if (year < 1 || month < 1 || month > 12 || day < 1)
            return false;

        return day <= daysInMonth(year, month);
    }

    bool isValidTime(const string& time)
    {
        if (time.size() != 8 || time[2] != ':' || time[5] != ':')
            return false;

        for (size_t i = 0; i < time.size(); i++)
        {
            if (i == 2 || i == 5) continue;
            if (time[i] < '0' || time[i] > '9') return false;
        }

        const int hour = (time[0] - '0') * 10 + (time[1] - '0');
        const int minute = (time[3] - '0') * 10 + (time[4] - '0');
        const int second = (time[6] - '0') * 10 + (time[7] - '0');

        return hour <= 23 && minute <= 59 && second <= 59;
    }

    string today()
    {
        const std::time_t now = std::time(nullptr);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        return std::to_string(local.tm_year + 1900) + "-" +
            twoDigits(local.tm_mon + 1) + "-" + twoDigits(local.tm_mday);
    }

    string nowTime()
    {
        const std::time_t now = std::time(nullptr);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        return twoDigits(local.tm_hour) + ":" + twoDigits(local.tm_min) + ":" +
            twoDigits(local.tm_sec);
    }

    int compareDates(const string& left, const string& right)
    {
        if (!isValidDate(left) || !isValidDate(right))
            throw std::invalid_argument("compareDates needs two YYYY-MM-DD dates");
        if (left < right) return -1;
        if (right < left) return 1;
        return 0;
    }
}
