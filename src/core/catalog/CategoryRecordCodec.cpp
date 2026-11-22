#include "CategoryRecordCodec.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

using std::invalid_argument;
using std::string;
using std::vector;

namespace
{
    bool isBlank(const string& s)
    {
        for (size_t i = 0; i < s.size(); i++)
            if (!std::isspace(static_cast<unsigned char>(s[i]))) return false;
        return true;
    }
}

vector<string> CategoryRecordCodec::splitRecords(const string& content)
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

ProductCategory CategoryRecordCodec::parse(const string& recordIn)
{
    // Accept the '#' marker as well, so format() output can be parsed directly.
    string record = recordIn;
    const size_t lastNonSpace = record.find_last_not_of(" \t\r\n");
    if (lastNonSpace != string::npos && record[lastNonSpace] == '#')
        record.erase(lastNonSpace);

    vector<string> tokens;
    std::istringstream input(record);
    string token;
    while (input >> token) tokens.push_back(token);

    if (tokens.size() < 3)
        throw invalid_argument("expected at least 3 fields but found " +
            std::to_string(tokens.size()));

    // The name may contain spaces on input; the acronym is the last field.
    const string& id = tokens[0];
    const string& acronym = tokens.back();

    string name;
    for (size_t i = 1; i + 1 < tokens.size(); i++)
    {
        if (!name.empty()) name += ' ';
        name += tokens[i];
    }
    for (size_t i = 0; i < name.size(); i++)
        if (name[i] == '_') name[i] = ' ';

    return ProductCategory(id, name, acronym);
}

string CategoryRecordCodec::format(const ProductCategory& category)
{
    string name = category.name();
    for (size_t i = 0; i < name.size(); i++)
        if (name[i] == ' ') name[i] = '_';

    return category.id() + " " + name + " " + category.acronym() + " #";
}
