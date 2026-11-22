#include "BillDetailRecordCodec.h"

#include "catalog/NumberFormat.h"
#include "catalog/Product.h"

#include <cctype>
#include <cmath>
#include <sstream>
#include <stdexcept>

using std::invalid_argument;
using std::string;
using std::vector;

namespace {

const std::size_t FIELD_COUNT = 5;

bool isBlank(const string& text)
{
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (!std::isspace(static_cast<unsigned char>(text[i])))
        {
            return false;
        }
    }

    return true;
}

bool isDigits(const string& text)
{
    if (text.empty())
    {
        return false;
    }

    for (char character : text)
    {
        if (character < '0' || character > '9')
        {
            return false;
        }
    }

    return true;
}

bool isPlainNumber(const string& text)
{
    bool digit = false;
    bool dot = false;

    for (char character : text)
    {
        if (character >= '0' && character <= '9')
        {
            digit = true;
        }
        else if (character == '.' && !dot)
        {
            dot = true;
        }
        else
        {
            return false;
        }
    }

    return digit;
}

double parseNonNegative(const string& text, const char* field)
{
    if (!isPlainNumber(text))
    {
        throw invalid_argument(string("invalid number for ") + field + ": '" + text + "'");
    }

    const double value = std::stod(text);
    if (value < 0)
    {
        throw invalid_argument(string(field) + " must be >= 0: '" + text + "'");
    }

    return value;
}

double parsePositive(const string& text, const char* field)
{
    const double value = parseNonNegative(text, field);
    if (value <= 0)
    {
        throw invalid_argument(string(field) + " must be > 0: '" + text + "'");
    }
    return value;
}

} // namespace

vector<string> BillDetailRecordCodec::splitRecords(const string& content)
{
    vector<string> records;

    const std::size_t lastMarker = content.rfind('#');
    const string tail = (lastMarker == string::npos) ? content : content.substr(lastMarker + 1);
    if (!isBlank(tail))
    {
        throw invalid_argument("the last record is missing the '#' end-of-line marker");
    }

    std::size_t start = 0;
    while (true)
    {
        const std::size_t end = content.find('#', start);
        if (end == string::npos)
        {
            break;
        }

        const string record = content.substr(start, end - start);
        if (!isBlank(record))
        {
            records.push_back(record);
        }
        start = end + 1;
    }

    return records;
}

BillLine BillDetailRecordCodec::parse(const string& recordIn)
{
    string record = recordIn;
    const std::size_t lastNonSpace = record.find_last_not_of(" \t\r\n");
    if (lastNonSpace != string::npos && record[lastNonSpace] == '#')
    {
        record.erase(lastNonSpace);
    }

    vector<string> tokens;
    std::istringstream input(record);
    string token;
    while (input >> token)
    {
        tokens.push_back(token);
    }

    if (tokens.size() != FIELD_COUNT)
    {
        throw invalid_argument("expected " + std::to_string(FIELD_COUNT) +
                               " fields but found " + std::to_string(tokens.size()));
    }

    BillLine line;

    if (tokens[0].size() != 9 || !isDigits(tokens[0]))
    {
        throw invalid_argument("bill id must be 9 digits: '" + tokens[0] + "'");
    }
    line.billId = tokens[0];

    if (tokens[1].empty() || tokens[1].size() > Product::maxAcronymLength() ||
        tokens[1].find('#') != string::npos)
    {
        throw invalid_argument("invalid product acronym: '" + tokens[1] + "'");
    }
    for (char character : tokens[1])
    {
        if (std::isspace(static_cast<unsigned char>(character)))
        {
            throw invalid_argument("invalid product acronym: '" + tokens[1] + "'");
        }
    }
    line.productAcronym = tokens[1];

    line.unitPrice = parseNonNegative(tokens[2], "unit price");
    line.quantity = parsePositive(tokens[3], "quantity");
    line.discountTotal = parseNonNegative(tokens[4], "discount total");

    const double subtotal = line.unitPrice * line.quantity;
    if (line.discountTotal > subtotal + 0.005)
    {
        throw invalid_argument(
            "line discount cannot exceed the line subtotal: '" + tokens[4] + "'");
    }

    return line;
}

string BillDetailRecordCodec::format(const BillLine& line)
{
    return line.billId + " " + line.productAcronym + " " +
           NumberFormat::general(line.unitPrice) + " " +
           NumberFormat::general(line.quantity) + " " +
           NumberFormat::general(line.discountTotal) + " #";
}
