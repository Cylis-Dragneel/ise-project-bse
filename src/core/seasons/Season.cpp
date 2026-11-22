#include "Season.h"

#include "util/DateUtils.h"

#include <cmath>
#include <stdexcept>

using std::invalid_argument;
using std::string;

bool Season::isValidDate(const string& date)
{
    return DateUtils::isValidDate(date);
}

Season::Season(const string& startDate,
               const string& endDate,
               double discountPercent,
               bool active)
    : startDate_(startDate), endDate_(endDate),
      discountPercent_(discountPercent), active_(active)
{
    if (!isValidDate(startDate_))
        throw invalid_argument("season start date must be YYYY-MM-DD, got '" +
            startDate_ + "'");
    if (!isValidDate(endDate_))
        throw invalid_argument("season end date must be YYYY-MM-DD, got '" +
            endDate_ + "'");
    if (endDate_ < startDate_)
        throw invalid_argument("season end date must not be before its start date");

    if (!std::isfinite(discountPercent_) || discountPercent_ < 0 ||
        discountPercent_ > maxDiscountPercent())
        throw invalid_argument("season discount percentage must be between 0 and 100");
}

bool Season::covers(const string& date) const
{
    if (!active_ || !isValidDate(date)) return false;
    return startDate_ <= date && date <= endDate_;
}

bool Season::overlaps(const Season& other) const
{
    if (!active_ || !other.active_) return false;
    return !(endDate_ < other.startDate_ || other.endDate_ < startDate_);
}
