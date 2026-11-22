#include "test_framework.h"

#include "seasons/FileSeasonRepository.h"
#include "seasons/Season.h"
#include "seasons/SeasonRecordCodec.h"
#include "seasons/SeasonService.h"

#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

const char *seasonPath = "test_seasons_tmp.txt";

std::string readFile(const std::string &path)
{
    std::ifstream file(path.c_str());
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

void writeFile(const std::string &path, const std::string &content)
{
    std::ofstream file(path.c_str(), std::ios::trunc);
    file << content;
}

Season makeDecember()
{
    return Season("2026-12-01", "2026-12-31", 10, true);
}

} // namespace

TEST_CASE(season_validates_calendar_dates)
{
    CHECK(Season::isValidDate("2026-12-31"));
    CHECK(Season::isValidDate("2024-02-29")); // leap year
    CHECK(!Season::isValidDate("2026-02-29")); // not a leap year
    CHECK(!Season::isValidDate("2026-13-01"));
    CHECK(!Season::isValidDate("2026-00-10"));
    CHECK(!Season::isValidDate("2026-01-32"));
    CHECK(!Season::isValidDate("2026-1-01"));
    CHECK(!Season::isValidDate("26-01-01"));
    CHECK(!Season::isValidDate(""));

    CHECK_THROWS_AS(Season("2026-12-31", "2026-12-01", 10, true),
                    std::invalid_argument);
    CHECK_THROWS_AS(Season("2026-12-01", "2026-12-31", -1, true),
                    std::invalid_argument);
    CHECK_THROWS_AS(Season("2026-12-01", "2026-12-31", 101, true),
                    std::invalid_argument);
}

TEST_CASE(season_record_round_trips)
{
    const Season original = makeDecember();
    const std::string record = SeasonRecordCodec::format(original);

    CHECK_EQ(record, std::string("2026-12-01 2026-12-31 10 true #"));

    const Season parsed = SeasonRecordCodec::parse(record);
    CHECK_EQ(parsed.startDate(), original.startDate());
    CHECK_EQ(parsed.endDate(), original.endDate());
    CHECK(parsed.discountPercent() == 10.0);
    CHECK(parsed.isActive());

    const Season inactive = SeasonRecordCodec::parse(
        "2026-11-01 2026-11-15 5.5 false #");
    CHECK(!inactive.isActive());
    CHECK(inactive.discountPercent() == 5.5);

    CHECK_THROWS_AS(SeasonRecordCodec::parse("2026-11-01 2026-11-15 5 maybe #"),
                    std::invalid_argument);
    CHECK_THROWS_AS(SeasonRecordCodec::parse("2026-11-01 2026-11-15 5 #"),
                    std::invalid_argument);
    CHECK_THROWS_AS(SeasonRecordCodec::splitRecords("2026-11-01 2026-11-15 5 true"),
                    std::invalid_argument);
}

TEST_CASE(season_file_repository_round_trips)
{
    std::remove(seasonPath);

    FileSeasonRepository repository(seasonPath);
    CHECK(repository.loadAll().empty());

    repository.saveAll({makeDecember(),
                        Season("2026-11-01", "2026-11-15", 5, false)});

    const std::vector<Season> loaded = repository.loadAll();
    CHECK_EQ(loaded.size(), static_cast<std::size_t>(2));
    CHECK_EQ(loaded[0].startDate(), std::string("2026-12-01"));
    CHECK(loaded[0].isActive());
    CHECK(!loaded[1].isActive());

    const std::string content = readFile(seasonPath);
    CHECK_EQ(content.back(), '\n');
    CHECK(content.find("2026-12-01 2026-12-31 10 true #") != std::string::npos);

    std::remove(seasonPath);
}

TEST_CASE(season_service_applies_the_active_season)
{
    std::remove(seasonPath);

    auto repository = std::make_shared<FileSeasonRepository>(seasonPath);
    SeasonService service(repository);
    service.load();

    service.add(makeDecember());

    CHECK(service.activePercent("2026-12-01") == 10.0);
    CHECK(service.activePercent("2026-12-31") == 10.0);
    CHECK(service.activePercent("2026-12-15") == 10.0);
    CHECK(service.activePercent("2026-11-30") == 0.0);
    CHECK(service.activePercent("2027-01-01") == 0.0);

    // Inactive seasons never apply.
    service.add(Season("2026-11-01", "2026-11-15", 5, false));
    CHECK(service.activePercent("2026-11-10") == 0.0);

    // Toggling it active makes it apply.
    service.setActive(1, true);
    CHECK(service.activePercent("2026-11-10") == 5.0);

    std::remove(seasonPath);
}

TEST_CASE(season_service_rejects_overlapping_active_seasons)
{
    std::remove(seasonPath);

    auto repository = std::make_shared<FileSeasonRepository>(seasonPath);
    SeasonService service(repository);
    service.load();

    service.add(Season("2026-12-01", "2026-12-31", 10, true));

    // Same range, active: rejected.
    CHECK_THROWS_AS(service.add(Season("2026-12-15", "2027-01-15", 5, true)),
                    std::invalid_argument);

    // Overlapping but inactive: allowed.
    service.add(Season("2026-12-15", "2027-01-15", 5, false));

    // Activating it would overlap: rejected and the stored value is unchanged.
    CHECK_THROWS_AS(service.setActive(1, true), std::invalid_argument);
    CHECK(!service.all()[1].isActive());

    // Adjacent ranges do not overlap.
    service.add(Season("2027-01-01", "2027-01-31", 5, true));

    std::remove(seasonPath);
}

TEST_CASE(season_service_rejects_overlaps_when_loading)
{
    writeFile(seasonPath,
              "2026-12-01 2026-12-31 10 true #\n"
              "2026-12-15 2027-01-15 5 true #\n");

    SeasonService service(std::make_shared<FileSeasonRepository>(seasonPath));
    CHECK_THROWS_AS(service.load(), std::invalid_argument);

    std::remove(seasonPath);
}

TEST_CASE(season_service_updates_and_removes)
{
    std::remove(seasonPath);

    auto repository = std::make_shared<FileSeasonRepository>(seasonPath);
    SeasonService service(repository);
    service.load();

    service.add(makeDecember());
    service.update(0, Season("2026-12-01", "2026-12-24", 15, true));
    CHECK(service.activePercent("2026-12-20") == 15.0);
    CHECK(service.activePercent("2026-12-25") == 0.0);

    service.remove(0);
    CHECK_EQ(service.all().size(), static_cast<std::size_t>(0));

    CHECK_THROWS_AS(service.update(0, makeDecember()), std::out_of_range);
    CHECK_THROWS_AS(service.remove(0), std::out_of_range);

    std::remove(seasonPath);
}

TEST_CASE(sample_seasons_load)
{
    const std::string path =
        std::string(QR_TEST_SOURCE_DIR) + "/data/Seasons.txt";

    SeasonService service(std::make_shared<FileSeasonRepository>(path));
    service.load();

    CHECK_EQ(service.all().size(), static_cast<std::size_t>(2));
    CHECK(service.activePercent("2026-12-25") == 10.0);
    CHECK(service.activePercent("2026-11-10") == 0.0); // the November season is inactive
}
