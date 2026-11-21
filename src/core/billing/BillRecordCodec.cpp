#include "BillRecordCodec.h"

#include "catalog/NumberFormat.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

using std::invalid_argument;
using std::string;
using std::vector;

namespace {

const std::size_t FIELD_COUNT = 10;

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

// Plain decimal numbers only: digits with at most one '.', e.g. 280, 72.5.
// No sign, no exponent (1e3), no hex, no "nan"/"inf".
bool isPlainNumber(const string& text, bool allowDot)
{
    bool digit = false;
    bool dot = false;

    for (char character : text)
    {
        if (character >= '0' && character <= '9')
        {
            digit = true;
        }
        else if (allowDot && character == '.' && !dot)
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

int parseProductId(const string& text)
{
    if (!isPlainNumber(text, false))
    {
        throw invalid_argument("invalid product id: '" + text + "'");
    }

    try
    {
        const int value = std::stoi(text);
        if (value <= 0)
        {
            throw invalid_argument("product id must be positive: '" + text + "'");
        }
        return value;
    }
    catch (const std::invalid_argument&)
    {
        throw;
    }
    catch (const std::exception&)
    {
        throw invalid_argument("product id is too large: '" + text + "'");
    }
}

double parseNonNegative(const string& text, const char* field)
{
    if (!isPlainNumber(text, true))
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

bool isValidStatus(const string& status)
{
    return status == "in-progress" || status == "on-hold" ||
           status == "paid" || status == "archived";
}

} // namespace

vector<string> BillRecordCodec::splitRecords(const string& content)
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

BillLine BillRecordCodec::parse(const string& recordIn)
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

    if (tokens[1].size() != 8 || !isDigits(tokens[1]) ||
        tokens[1].substr(0, 4) == "0000")
    {
        throw invalid_argument("order id must be 8 digits: '" + tokens[1] + "'");
    }
    line.orderId = tokens[1];

    line.productId = parseProductId(tokens[2]);

    if (tokens[3].size() != 6 || !isDigits(tokens[3]) || tokens[3][0] == '0')
    {
        throw invalid_argument("product barcode must be 6 digits from 100000 to 999999: '" +
                               tokens[3] + "'");
    }
    line.productBarcode = tokens[3];

    line.quantity = parsePositive(tokens[4], "quantity");
    line.unitPrice = parseNonNegative(tokens[5], "unit price");
    line.taxPerUnit = parseNonNegative(tokens[6], "tax");
    line.discountPerUnit = parseNonNegative(tokens[7], "discount");
    line.lineTotal = parseNonNegative(tokens[8], "line total");

    if (!isValidStatus(tokens[9]))
    {
        throw invalid_argument("invalid payment status: '" + tokens[9] + "'");
    }
    line.paymentStatus = tokens[9];

    return line;
}

string BillRecordCodec::format(const BillLine& line)
{
    return line.billId + " " + line.orderId + " " +
           std::to_string(line.productId) + " " + line.productBarcode + " " +
           NumberFormat::general(line.quantity) + " " +
           NumberFormat::general(line.unitPrice) + " " +
           NumberFormat::general(line.taxPerUnit) + " " +
           NumberFormat::general(line.discountPerUnit) + " " +
           NumberFormat::general(line.lineTotal) + " " +
           line.paymentStatus + " #";
}
