#include "orderPolicy.h"

namespace {

bool parseDate(const std::string &date, int &year, int &month, int &day)
{
    if (date.size() != 10 || date[4] != '-' || date[7] != '-')
    {
        return false;
    }

    for (std::size_t i = 0; i < date.size(); ++i)
    {
        if (i == 4 || i == 7)
        {
            continue;
        }
        if (date[i] < '0' || date[i] > '9')
        {
            return false;
        }
    }

    year = std::stoi(date.substr(0, 4));
    month = std::stoi(date.substr(5, 2));
    day = std::stoi(date.substr(8, 2));

    if (month < 1 || month > 12 || day < 1)
    {
        return false;
    }

    static const int days[] = {31, 28, 31, 30, 31, 30,
                               31, 31, 30, 31, 30, 31};
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    const int limit = (month == 2 && leap) ? 29 : days[month - 1];
    return day <= limit;
}

// Days since 1970-01-01 (proleptic Gregorian), so dates subtract correctly
// across month and year ends.
long daysFromCivil(int year, int month, int day)
{
    year -= month <= 2;
    const long era = (year >= 0 ? year : year - 399) / 400;
    const long yearOfEra = year - era * 400;
    const long dayOfYear =
        (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const long dayOfEra =
        yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + dayOfEra - 719468;
}

bool dayNumber(const std::string &date, long &number)
{
    int year = 0;
    int month = 0;
    int day = 0;
    if (!parseDate(date, year, month, day))
    {
        return false;
    }
    number = daysFromCivil(year, month, day);
    return true;
}

} // namespace

bool OrderPolicy::isUnpaid(const std::string &status)
{
    return status == "in-progress" || status == "on-hold";
}

bool OrderPolicy::canTransition(const std::string &from, const std::string &to)
{
    if (from == to)
    {
        return from == "in-progress" || from == "on-hold" ||
               from == "paid" || from == "archived";
    }

    if (from == "in-progress")
    {
        return to == "on-hold" || to == "paid" || to == "archived";
    }
    if (from == "on-hold")
    {
        return to == "in-progress" || to == "paid" || to == "archived";
    }
    if (from == "paid")
    {
        return to == "archived";
    }

    return false; // archived is final; unknown statuses go nowhere
}

bool OrderPolicy::isWithinPaidEditWindow(const Order &order,
                                         const BillStamp &bill,
                                         const std::string &today)
{
    const std::string &stampDate =
        (bill.hasBill && !bill.date.empty()) ? bill.date : order.date;

    long stamp = 0;
    long now = 0;
    if (!dayNumber(stampDate, stamp) || !dayNumber(today, now))
    {
        return false;
    }

    // A stamp in the future (clock change) is treated as recent.
    return now - stamp <= kPaidEditWindowDays;
}

bool OrderPolicy::requiresManagerApproval(const Order &order,
                                          const BillStamp &bill,
                                          const std::string &today)
{
    return order.status == "paid" &&
           isWithinPaidEditWindow(order, bill, today);
}

bool OrderPolicy::canEdit(const Order &order,
                          const BillStamp &bill,
                          const std::string &today,
                          bool managerApproved)
{
    if (isUnpaid(order.status))
    {
        return true;
    }

    if (order.status == "paid")
    {
        return isWithinPaidEditWindow(order, bill, today) && managerApproved;
    }

    return false; // archived or unknown
}
