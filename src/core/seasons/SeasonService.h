#pragma once
#ifndef SEASON_SERVICE_H
#define SEASON_SERVICE_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "ISeasonProvider.h"
#include "ISeasonRepository.h"
#include "Season.h"

// The business view of Seasons.txt: add/edit/toggle seasons and find the
// discount that applies on a given date.
//
// Decision: two ACTIVE seasons may not overlap. This keeps "the" seasonal
// discount unambiguous at billing time. Inactive seasons may overlap freely.
class SeasonService : public ISeasonProvider
{
private:
    std::shared_ptr<ISeasonRepository> repository_;
    std::vector<Season> seasons_;

    void rejectOverlaps(const Season& candidate, std::size_t ignoreIndex) const;
    void saveWithRollback(const std::vector<Season>& backup);

public:
    explicit SeasonService(std::shared_ptr<ISeasonRepository> repository);

    // Loads all seasons. Throws std::invalid_argument when two active seasons
    // overlap, so a hand-edited file cannot silently produce two discounts.
    void load();

    const std::vector<Season>& all() const { return seasons_; }

    // Adds a season and persists it. Throws std::invalid_argument when it
    // overlaps another active season.
    void add(const Season& season);

    // Replaces seasons_[index] and persists it.
    void update(std::size_t index, const Season& season);

    // Convenience for the manager's active/inactive toggle.
    void setActive(std::size_t index, bool active);

    // Removes seasons_[index] and persists the list.
    void remove(std::size_t index);

    // Percentage of the active season that covers `date`, or 0 when none does.
    // Throws std::invalid_argument for an invalid date or (defensively) when
    // more than one active season matches.
    double activePercent(const std::string& date) const override;
};

#endif // SEASON_SERVICE_H
