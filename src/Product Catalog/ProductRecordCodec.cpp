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
    const size_t FIELDS_BEFORE_NAME = 3; // id, barcode, acronym
    const size_t FIELDS_AFTER_NAME = 6; // unit, price, tax, discount, quantity, path

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

Product ProductRecordCodec::parse(const string& record)
{
    vector<string> t;
    std::istringstream in(record);
    string token;
    while (in >> token) t.push_back(token);

    const size_t minFields = FIELDS_BEFORE_NAME + 1 + FIELDS_AFTER_NAME;
    if (t.size() < minFields)
        throw invalid_argument("expected at least " + std::to_string(minFields) +
            " fields but found " + std::to_string(t.size()));

    // The name may contain spaces, so it is everything between the first 3
    // and the last 6 fields.
    const size_t nameEnd = t.size() - FIELDS_AFTER_NAME;
    string name;
    for (size_t i = FIELDS_BEFORE_NAME; i < nameEnd; i++)
    {
        if (!name.empty()) name += ' ';
        name += t[i];
    }
    for (size_t i = 0; i < name.size(); i++)
        if (name[i] == '_') name[i] = ' ';

    return Product(parseInt(t[0], "product id"),
        t[1],
        t[2],
        name,
        measurementUnitFromString(t[nameEnd]),
        parseDouble(t[nameEnd + 1], "unit price"),
        parseDouble(t[nameEnd + 2], "tax"),
        parseDouble(t[nameEnd + 3], "discount"),
        parseDouble(t[nameEnd + 4], "available quantity"),
        t[nameEnd + 5]);
}

string ProductRecordCodec::format(const Product& p)
{
    string name = p.name();
    for (size_t i = 0; i < name.size(); i++)
        if (name[i] == ' ') name[i] = '_';

    return std::to_string(p.id()) + " " + p.barcode() + " " + p.acronym() + " " +
        name + " " + toString(p.unit()) + " " +
        NumberFormat::general(p.unitPrice()) + " " +
        NumberFormat::general(p.taxAmount()) + " " +
        NumberFormat::general(p.discountPerUnit()) + " " +
        NumberFormat::general(p.availableQuantity()) + " " +
        p.barcodeImagePath() + "#";
}