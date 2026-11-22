#include "ProductRecordCodec.h"
#include "NumberFormat.h"
#include <sstream>
#include <stdexcept>
#include <cctype>

using std::invalid_argument;
using std::string;
using std::vector;

namespace
{
    const size_t FIELDS_AFTER_UNIT = 5; // price, tax, discount, quantity, path

    bool isTwoDigits(const string& text)
    {
        return text.size() == 2 &&
            std::isdigit(static_cast<unsigned char>(text[0])) &&
            std::isdigit(static_cast<unsigned char>(text[1]));
    }

    bool isBlank(const string& s)
    {
        for (size_t i = 0; i < s.size(); i++)
            if (!std::isspace(static_cast<unsigned char>(s[i]))) return false;
        return true;
    }

    // Plain decimal numbers only: digits with at most one '.', e.g. 280, 72.5.
    // No sign, no exponent (1e3), no hex (0x10), no "nan"/"inf".
    bool isPlainNumber(const string& s, bool allowDot)
    {
        bool digit = false, dot = false;
        for (size_t i = 0; i < s.size(); i++)
        {
            if (std::isdigit(static_cast<unsigned char>(s[i]))) digit = true;
            else if (allowDot && s[i] == '.' && !dot) dot = true;
            else return false;
        }
        return digit;
    }

    int parseInt(const string& text, const char* field)
    {
        if (!isPlainNumber(text, false))
            throw invalid_argument(string("invalid whole number for ") + field + ": '" + text + "'");
        try { return std::stoi(text); }
        catch (const std::exception&)
        {
            throw invalid_argument(string(field) + " is too large: '" + text + "'");
        }
    }

    double parseDouble(const string& text, const char* field)
    {
        if (!isPlainNumber(text, true))
            throw invalid_argument(string("invalid number for ") + field + ": '" + text +
                "' (use plain digits like 280 or 72.5)");
        return std::stod(text);
    }
}

vector<string> ProductRecordCodec::splitRecords(const string& content)
{
    vector<string> records;
    size_t lastMarker = content.rfind('#');
    string tail = (lastMarker == string::npos) ? content : content.substr(lastMarker + 1);
    if (!isBlank(tail))
        throw invalid_argument("the last record is missing the '#' end-of-line marker");

    size_t start = 0;
    while (true)
    {
        size_t end = content.find('#', start);
        if (end == string::npos) break;
        string record = content.substr(start, end - start);
        if (!isBlank(record)) records.push_back(record);
        start = end + 1;
    }
    return records;
}

Product ProductRecordCodec::parse(const string& recordIn)
{
    // Accept the '#' marker as well, so format() output can be parsed directly.
    string record = recordIn;
    const size_t lastNonSpace = record.find_last_not_of(" \t\r\n");
    if (lastNonSpace != string::npos && record[lastNonSpace] == '#')
        record.erase(lastNonSpace);

    vector<string> t;
    std::istringstream in(record);
    string token;
    while (in >> token) t.push_back(token);

    // The measurement unit is the only token that can be "count"/"weight".
    size_t unitIndex = t.size();
    for (size_t i = 0; i < t.size(); i++)
    {
        if (t[i] == "count" || t[i] == "weight")
        {
            unitIndex = i;
            break;
        }
    }
    if (unitIndex == t.size())
        throw invalid_argument("missing measurement unit ('count' or 'weight')");
    if (t.size() != unitIndex + 1 + FIELDS_AFTER_UNIT)
        throw invalid_argument("expected " + std::to_string(unitIndex + 1 + FIELDS_AFTER_UNIT) +
            " fields but found " + std::to_string(t.size()));

    // New records carry categoryId at index 2 and the acronym at index 3.
    // Legacy records have the acronym at index 2; the category is inferred from
    // the barcode prefix. A new record always has at least five tokens before
    // the unit (id, barcode, categoryId, acronym, one name word).
    const bool hasCategory = unitIndex >= 4 && isTwoDigits(t[2]);
    const size_t acronymIndex = hasCategory ? 3 : 2;
    const size_t nameStart = acronymIndex + 1;
    if (unitIndex <= nameStart)
        throw invalid_argument("product name is missing");

    // The name may contain spaces, so it is everything between the acronym and
    // the measurement unit.
    string name;
    for (size_t i = nameStart; i < unitIndex; i++)
    {
        if (!name.empty()) name += ' ';
        name += t[i];
    }
    for (size_t i = 0; i < name.size(); i++)
        if (name[i] == '_') name[i] = ' ';

    const string categoryId = hasCategory ? t[2] : t[1].substr(0, 2);

    return Product(parseInt(t[0], "product id"),
        t[1],
        categoryId,
        t[acronymIndex],
        name,
        measurementUnitFromString(t[unitIndex]),
        parseDouble(t[unitIndex + 1], "unit price"),
        parseDouble(t[unitIndex + 2], "tax"),
        parseDouble(t[unitIndex + 3], "discount"),
        parseDouble(t[unitIndex + 4], "available quantity"),
        t[unitIndex + 5]);
}

string ProductRecordCodec::format(const Product& p)
{
    string name = p.name();
    for (size_t i = 0; i < name.size(); i++)
        if (name[i] == ' ') name[i] = '_';

    return std::to_string(p.id()) + " " + p.barcode() + " " + p.categoryId() + " " +
        p.acronym() + " " + name + " " + measurementUnitToString(p.unit()) + " " +
        NumberFormat::general(p.unitPrice()) + " " +
        NumberFormat::general(p.taxAmount()) + " " +
        NumberFormat::general(p.discountPerUnit()) + " " +
        NumberFormat::general(p.availableQuantity()) + " " +
        p.barcodeImagePath() + "#";
}