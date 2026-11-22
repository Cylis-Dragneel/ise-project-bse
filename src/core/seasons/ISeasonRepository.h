#pragma once
#ifndef I_SEASON_REPOSITORY_H
#define I_SEASON_REPOSITORY_H

#include <vector>

#include "Season.h"

// Abstraction of "where seasons are stored". SeasonService depends only on this
// interface.
class ISeasonRepository
{
public:
    virtual ~ISeasonRepository() = default;
    virtual std::vector<Season> loadAll() = 0;
    virtual void saveAll(const std::vector<Season>& seasons) = 0;
};

#endif // I_SEASON_REPOSITORY_H
