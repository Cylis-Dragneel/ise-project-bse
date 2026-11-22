#pragma once
#ifndef I_SEASON_PROVIDER_H
#define I_SEASON_PROVIDER_H

#include <string>

// The only thing billing needs from the season layer: "what percentage applies
// on this date?". Keeping it an interface lets billing tests inject a fixed
// season without touching a file.
class ISeasonProvider
{
public:
    virtual ~ISeasonProvider() = default;

    // 0 when no active season covers the date.
    virtual double activePercent(const std::string& date) const = 0;
};

#endif // I_SEASON_PROVIDER_H
