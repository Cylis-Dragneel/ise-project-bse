#ifndef CATEGORY_RECORD_CODEC_H
#define CATEGORY_RECORD_CODEC_H

#include <string>
#include <vector>

#include "ProductCategory.h"

// Knows ONLY the text format of PCategory.txt. No file access here.
//
// One record:
//   categoryId name acronym #
//
// The name is written with '_' instead of spaces; both forms are accepted when
// reading.
class CategoryRecordCodec
{
public:
    // Splits the whole file content into records (text before each '#').
    // Throws std::invalid_argument if the last record has no '#'.
    static std::vector<std::string> splitRecords(const std::string& fileContent);

    // Parses ONE record (without the '#'). A trailing '#' is also accepted.
    // Throws std::invalid_argument.
    static ProductCategory parse(const std::string& record);

    // One record including the '#' marker (no newline).
    static std::string format(const ProductCategory& category);
};

#endif // CATEGORY_RECORD_CODEC_H
