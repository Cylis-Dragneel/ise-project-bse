#include "SeasonRecordCodec.h"

#include "catalog/NumberFormat.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

using std::invalid_argument;
using std::string;
using std::vector;

namespace
{
    bool isBlank(const string& text)
    {
        for (size_t i = 0; i < text.size(); i++)
            if (!std::isspace(static_cast<unsigned char>(text[i]))) return false;
        return true;
    }

    bool isPlainNumber(const string& text)
    {
        bool digit = false, dot = false;
        for (size_t i = 0; i < text.size(); i++)
        {
            if (std::isdigit(static_cast<unsigned char>(text[i]))) digit = true;
            else if (text[i] == '.' && !dot) dot = true;
            else return false;
        }
        return digit;
    }
}

vector<string> SeasonRecordCodec::splitRecords(const string& content)
{
    vector<string> records;
    const size_t lastMarker = content.rfind('#');
    const string tail = (lastMarker == string::npos) ? content : content.substr(lastMarker + 1);
    if (!isBlank(tail))
        throw invalid_argument("the last record is missing the '#' end-of-line marker");

    size_t start = 0;
    while (true)
    {
        const size_t end = content.find('#', start);
        if (end == string::npos) break;
        const string record = content.substr(start, end - start);
        if (!isBlank(record)) records.push_back(record);
        start = end + 1;
    }
    return records;
}

Season SeasonRecordCodec::parse(const string& recordIn)
{
    string record = recordIn;
    const size_t lastNonSpace = record.find_last_not_of(" \t\r\n");
    if (lastNonSpace != string::npos && record[lastNonSpace] == '#')
        record.erase(lastNonSpace);

    vector<string> tokens;
    std::istringstream input(record);
    string token;
    while (input >> token) tokens.push_back(token);

    if (tokens.size() != 4)
        throw invalid_argument("expected 4 fields but found " +
            std::to_string(tokens.size()));

    if (!isPlainNumber(tokens[2]))
        throw invalid_argument("invalid discount percentage: '" + tokens[2] + "'");

    bool active = false;
    if (tokens[3] == "true") active = true;
    else if (tokens[3] != "false")
        throw invalid_argument("isActive must be 'true' or 'false', got '" +
            tokens[3] + "'");

    return Season(tokens[0], tokens[1], std::stod(tokens[2]), active);
}

string SeasonRecordCodec::format(const Season& season)
{
    return season.startDate() + " " + season.endDate() + " " +
        NumberFormat::general(season.discountPercent()) + " " +
        (season.isActive() ? "true" : "false") + " #";
}
