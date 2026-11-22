#ifndef SEASON_H
#define SEASON_H

#include <string>

// One row of Seasons.txt:
//   startDate endDate discountPercent isActive #
//
// Dates are ISO YYYY-MM-DD. A season applies to a bill when it is active and
// the bill date is between startDate and endDate (inclusive).
class Season
{
private:
    std::string startDate_;
    std::string endDate_;
    double discountPercent_;
    bool active_;

public:
    static double maxDiscountPercent() { return 100.0; }

    // True for a real calendar date in YYYY-MM-DD form.
    static bool isValidDate(const std::string& date);

    // Throws std::invalid_argument for a bad date, start > end, a percentage
    // outside 0-100, or a non-finite percentage.
    Season(const std::string& startDate,
           const std::string& endDate,
           double discountPercent,
           bool active);

    const std::string& startDate() const { return startDate_; }
    const std::string& endDate() const { return endDate_; }
    double discountPercent() const { return discountPercent_; }
    bool isActive() const { return active_; }

    // True when active and startDate <= date <= endDate.
    bool covers(const std::string& date) const;

    // True when both seasons are active and their date ranges intersect.
    bool overlaps(const Season& other) const;
};

#endif // SEASON_H
