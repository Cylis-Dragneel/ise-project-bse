#ifndef SEASON_RECORD_CODEC_H
#define SEASON_RECORD_CODEC_H

#include <string>
#include <vector>

#include "Season.h"

// Knows ONLY the text format of Seasons.txt. No file access here.
//
// One record:
//   startDate endDate discountPercent isActive #
//
// isActive is exactly "true" or "false".
class SeasonRecordCodec
{
public:
    // Splits the whole file content into records (text before each '#').
    // Throws std::invalid_argument if the last record has no '#'.
    static std::vector<std::string> splitRecords(const std::string& fileContent);

    // Parses ONE record (without the '#'). A trailing '#' is also accepted.
    // Throws std::invalid_argument.
    static Season parse(const std::string& record);

    // One record including the '#' marker (no newline).
    static std::string format(const Season& season);
};

#endif // SEASON_RECORD_CODEC_H
