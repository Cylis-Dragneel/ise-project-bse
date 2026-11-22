#include "BillRecordCodec.h"

#include "catalog/NumberFormat.h"
#include "util/DateUtils.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

using std::invalid_argument;
using std::string;
using std::vector;

namespace {

const std::size_t FIELD_COUNT = 14;

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

bool isValidStatus(const string& status)
{
    return status == "in-progress" || status == "on-hold" ||
           status == "paid" || status == "archived";
}

// The file stores a customer name without spaces; an empty name is "-".
string parseCustomerName(const string& token)
{
    if (token == "-") return string();
    string name = token;
    for (std::size_t i = 0; i < name.size(); ++i)
    {
        if (name[i] == '_') name[i] = ' ';
    }
    return name;
}

string formatCustomerName(const string& name)
{
    if (name.empty()) return "-";
    string token = name;
    for (std::size_t i = 0; i < token.size(); ++i)
    {
        if (token[i] == ' ') token[i] = '_';
    }
    return token;
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

Bill BillRecordCodec::parse(const string& recordIn)
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

    Bill bill;

    if (tokens[0].size() != 9 || !isDigits(tokens[0]))
    {
        throw invalid_argument("bill id must be 9 digits: '" + tokens[0] + "'");
    }
    bill.billId = tokens[0];

    bill.customerName = parseCustomerName(tokens[1]);

    if (tokens[2].size() != 8 || !isDigits(tokens[2]) ||
        tokens[2].substr(0, 4) == "0000")
    {
        throw invalid_argument("order id must be 8 digits: '" + tokens[2] + "'");
    }
    bill.orderId = tokens[2];

    bill.totalNoTax = parseNonNegative(tokens[3], "total without tax");
    bill.totalTax = parseNonNegative(tokens[4], "total tax");
    bill.totalDiscount = parseNonNegative(tokens[5], "total discount");
    bill.totalPayable = parseNonNegative(tokens[6], "total payable");
    bill.amountReceived = parseNonNegative(tokens[7], "amount received");
    bill.balanceReturned = parseNonNegative(tokens[8], "balance returned");

    if (!isValidStatus(tokens[9]))
    {
        throw invalid_argument("invalid payment status: '" + tokens[9] + "'");
    }
    bill.paymentStatus = tokens[9];

    bill.seasonalDiscount = parseNonNegative(tokens[10], "seasonal discount");

    bill.specialDiscountPercent = parseNonNegative(tokens[11], "special discount");
    if (bill.specialDiscountPercent > 8)
    {
        throw invalid_argument("special discount must be at most 8 percent: '" +
                               tokens[11] + "'");
    }

    if (!DateUtils::isValidDate(tokens[12]))
    {
        throw invalid_argument("bill date must be YYYY-MM-DD: '" + tokens[12] + "'");
    }
    bill.billDate = tokens[12];

    if (!DateUtils::isValidTime(tokens[13]))
    {
        throw invalid_argument("bill time must be HH:MM:SS: '" + tokens[13] + "'");
    }
    bill.billTime = tokens[13];

    return bill;
}

string BillRecordCodec::format(const Bill& bill)
{
    return bill.billId + " " + formatCustomerName(bill.customerName) + " " +
           bill.orderId + " " +
           NumberFormat::general(bill.totalNoTax) + " " +
           NumberFormat::general(bill.totalTax) + " " +
           NumberFormat::general(bill.totalDiscount) + " " +
           NumberFormat::general(bill.totalPayable) + " " +
           NumberFormat::general(bill.amountReceived) + " " +
           NumberFormat::general(bill.balanceReturned) + " " +
           bill.paymentStatus + " " +
           NumberFormat::general(bill.seasonalDiscount) + " " +
           NumberFormat::general(bill.specialDiscountPercent) + " " +
           bill.billDate + " " + bill.billTime + " #";
}
