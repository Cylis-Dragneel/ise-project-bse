#include "SeasonService.h"

#include <stdexcept>

using std::string;
using std::vector;

SeasonService::SeasonService(std::shared_ptr<ISeasonRepository> repository)
    : repository_(std::move(repository))
{
    if (!repository_)
        throw std::invalid_argument("SeasonService needs a repository");
}

void SeasonService::rejectOverlaps(const Season& candidate,
                                   std::size_t ignoreIndex) const
{
    for (std::size_t i = 0; i < seasons_.size(); i++)
    {
        if (i == ignoreIndex) continue;
        if (candidate.overlaps(seasons_[i]))
            throw std::invalid_argument(
                "season " + candidate.startDate() + ".." + candidate.endDate() +
                " overlaps active season " + seasons_[i].startDate() + ".." +
                seasons_[i].endDate());
    }
}

void SeasonService::saveWithRollback(const vector<Season>& backup)
{
    try
    {
        repository_->saveAll(seasons_);
    }
    catch (...)
    {
        seasons_ = backup;
        throw;
    }
}

void SeasonService::load()
{
    vector<Season> loaded = repository_->loadAll();

    // Reject overlaps before replacing the current list.
    for (std::size_t i = 0; i < loaded.size(); i++)
    {
        if (!loaded[i].isActive()) continue;
        for (std::size_t j = i + 1; j < loaded.size(); j++)
        {
            if (loaded[i].overlaps(loaded[j]))
                throw std::invalid_argument(
                    "overlapping active seasons: " + loaded[i].startDate() + ".." +
                    loaded[i].endDate() + " and " + loaded[j].startDate() + ".." +
                    loaded[j].endDate());
        }
    }

    seasons_.swap(loaded);
}

void SeasonService::add(const Season& season)
{
    rejectOverlaps(season, seasons_.size());

    const vector<Season> backup = seasons_;
    seasons_.push_back(season);
    saveWithRollback(backup);
}

void SeasonService::update(std::size_t index, const Season& season)
{
    if (index >= seasons_.size())
        throw std::out_of_range("season index out of range");

    rejectOverlaps(season, index);

    const vector<Season> backup = seasons_;
    seasons_[index] = season;
    saveWithRollback(backup);
}

void SeasonService::setActive(std::size_t index, bool active)
{
    if (index >= seasons_.size())
        throw std::out_of_range("season index out of range");

    const Season& current = seasons_[index];
    update(index, Season(current.startDate(), current.endDate(),
                         current.discountPercent(), active));
}

void SeasonService::remove(std::size_t index)
{
    if (index >= seasons_.size())
        throw std::out_of_range("season index out of range");

    const vector<Season> backup = seasons_;
    seasons_.erase(seasons_.begin() + static_cast<std::ptrdiff_t>(index));
    saveWithRollback(backup);
}

double SeasonService::activePercent(const string& date) const
{
    if (!Season::isValidDate(date))
        throw std::invalid_argument("invalid bill date: '" + date + "'");

    const Season* match = nullptr;
    for (const Season& season : seasons_)
    {
        if (!season.covers(date)) continue;
        if (match != nullptr)
            throw std::invalid_argument(
                "more than one active season covers " + date);
        match = &season;
    }

    return match == nullptr ? 0.0 : match->discountPercent();
}
