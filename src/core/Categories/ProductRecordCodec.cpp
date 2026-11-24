#include "ProductRecordCodec.h"
#include "FieldRules.h"
#include "NumberFormat.h"
#include "RecordSplitter.h"
#include <stdexcept>

using std::invalid_argument;
using std::string;
using std::vector;

namespace
{
    const size_t FIELDS_AFTER_NAME = 6;

    bool isPlainNumber(const string& s, bool allowDot)
    {
        bool digit = false, dot = false;
        for (size_t i = 0; i < s.size(); i++)
        {
            if (s[i] >= '0' && s[i] <= '9') digit = true;
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

    bool looksLikeCategoryId(const string& token)
    {
        return FieldRules::isDigitsOnly(token);
    }
}

vector<string> ProductRecordCodec::splitRecords(const string& fileContent)
{
    return RecordSplitter::split(fileContent);
}

Product ProductRecordCodec::parse(const string& record)
{
    const vector<string> t = RecordSplitter::tokens(record);

    const bool hasCategory = t.size() >= 3 && looksLikeCategoryId(t[2]);
    const size_t acronymIndex = hasCategory ? 3 : 2;
    const size_t nameStart = acronymIndex + 1;
    const size_t minFields = nameStart + 1 + FIELDS_AFTER_NAME;

    if (t.size() < minFields)
        throw invalid_argument("expected at least " + std::to_string(minFields) +
            " fields but found " + std::to_string(t.size()));

    const size_t nameEnd = t.size() - FIELDS_AFTER_NAME;
    string name;
    for (size_t i = nameStart; i < nameEnd; i++)
    {
        if (!name.empty()) name += ' ';
        name += t[i];
    }
    for (size_t i = 0; i < name.size(); i++)
        if (name[i] == '_') name[i] = ' ';

    const string categoryId = hasCategory ? t[2] : t[1].substr(0, Product::categoryIdLength());

    return Product(parseInt(t[0], "product id"),
        t[1],
        categoryId,
        t[acronymIndex],
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

    return RecordSplitter::endRecord(
        std::to_string(p.id()) + " " + p.barcode() + " " + p.categoryId() + " " +
        p.acronym() + " " + name + " " + toString(p.unit()) + " " +
        NumberFormat::general(p.unitPrice()) + " " +
        NumberFormat::general(p.taxAmount()) + " " +
        NumberFormat::general(p.discountPerUnit()) + " " +
        NumberFormat::general(p.availableQuantity()) + " " +
        p.barcodeImagePath());
}