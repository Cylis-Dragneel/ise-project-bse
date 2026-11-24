#include "BillRecordCodec.h"

#include "catalog/NumberFormat.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

using std::invalid_argument;
using std::string;
using std::vector;

namespace {

const std::size_t HEADER_FIELD_COUNT = 14;
const std::size_t DETAIL_FIELD_COUNT = 5;

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

vector<string> splitHashRecords(const string& content)
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

vector<string> tokenizeRecord(const string& recordIn)
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
    return tokens;
}

bool isValidDateFormat(const string& d)
{
    if (d.size() != 10 || d[4] != '-' || d[7] != '-')
    {
        return false;
    }
    for (std::size_t i = 0; i < d.size(); ++i)
    {
        if (i == 4 || i == 7) continue;
        if (d[i] < '0' || d[i] > '9') return false;
    }
    return true;
}

bool isValidTimeFormat(const string& t)
{
    if (t.size() != 8 || t[2] != ':' || t[5] != ':')
    {
        return false;
    }
    for (std::size_t i = 0; i < t.size(); ++i)
    {
        if (i == 2 || i == 5) continue;
        if (t[i] < '0' || t[i] > '9') return false;
    }
    return true;
}

} // namespace

string BillRecordCodec::sanitizeCustomerName(const string& rawName)
{
    const std::size_t first = rawName.find_first_not_of(" \t\r\n");
    if (first == string::npos)
    {
        return "-";
    }
    const std::size_t last = rawName.find_last_not_of(" \t\r\n");
    string clean = rawName.substr(first, last - first + 1);
    if (clean.empty())
    {
        return "-";
    }
    for (char& ch : clean)
    {
        if (std::isspace(static_cast<unsigned char>(ch)))
        {
            ch = '_';
        }
    }
    return clean;
}

vector<string> BillRecordCodec::splitRecords(const string& content)
{
    return splitHashRecords(content);
}

string BillRecordCodec::format(const Bill& bill)
{
    return bill.billId + " " +
           sanitizeCustomerName(bill.customerName) + " " +
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
           bill.billDate + " " +
           bill.billTime + " #";
}

Bill BillRecordCodec::parse(const string& recordIn)
{
    const vector<string> tokens = tokenizeRecord(recordIn);

    if (tokens.size() != HEADER_FIELD_COUNT)
    {
        throw invalid_argument("expected " + std::to_string(HEADER_FIELD_COUNT) +
                               " fields in Bill.txt but found " + std::to_string(tokens.size()));
    }

    Bill bill;

    if (tokens[0].size() != 9 || !isDigits(tokens[0]))
    {
        throw invalid_argument("bill id must be 9 digits: '" + tokens[0] + "'");
    }
    bill.billId = tokens[0];

    bill.customerName = tokens[1];

    if (tokens[2].size() != 8 || !isDigits(tokens[2]) || tokens[2].substr(0, 4) == "0000")
    {
        throw invalid_argument("order id must be 8 digits: '" + tokens[2] + "'");
    }
    bill.orderId = tokens[2];

    bill.totalNoTax = parseNonNegative(tokens[3], "totalNoTax");
    bill.totalTax = parseNonNegative(tokens[4], "totalTax");
    bill.totalDiscount = parseNonNegative(tokens[5], "totalDiscount");
    bill.totalPayable = parseNonNegative(tokens[6], "totalPayable");
    bill.amountReceived = parseNonNegative(tokens[7], "amountReceived");
    bill.balanceReturned = parseNonNegative(tokens[8], "balanceReturned");

    if (!isValidStatus(tokens[9]))
    {
        throw invalid_argument("invalid payment status: '" + tokens[9] + "'");
    }
    bill.paymentStatus = tokens[9];

    bill.seasonalDiscount = parseNonNegative(tokens[10], "seasonalDiscount");
    bill.specialDiscountPercent = parseNonNegative(tokens[11], "specialDiscount");
    if (bill.specialDiscountPercent > 8.0)
    {
        throw invalid_argument("special discount cannot exceed 8%: '" + tokens[11] + "'");
    }

    if (!isValidDateFormat(tokens[12]))
    {
        throw invalid_argument("invalid bill date (expected YYYY-MM-DD): '" + tokens[12] + "'");
    }
    bill.billDate = tokens[12];

    if (!isValidTimeFormat(tokens[13]))
    {
        throw invalid_argument("invalid bill time (expected HH:MM:SS): '" + tokens[13] + "'");
    }
    bill.billTime = tokens[13];

    return bill;
}

vector<string> BillDetailRecordCodec::splitRecords(const string& content)
{
    return splitHashRecords(content);
}

string BillDetailRecordCodec::format(const BillLine& line)
{
    return line.billId + " " +
           line.productAcronym + " " +
           NumberFormat::general(line.unitPrice) + " " +
           NumberFormat::general(line.quantity) + " " +
           NumberFormat::general(line.discountTotal) + " #";
}

BillLine BillDetailRecordCodec::parse(const string& recordIn)
{
    const vector<string> tokens = tokenizeRecord(recordIn);

    if (tokens.size() != DETAIL_FIELD_COUNT)
    {
        throw invalid_argument("expected " + std::to_string(DETAIL_FIELD_COUNT) +
                               " fields in BillDetail.txt but found " + std::to_string(tokens.size()));
    }

    BillLine line;

    if (tokens[0].size() != 9 || !isDigits(tokens[0]))
    {
        throw invalid_argument("bill id in BillDetail.txt must be 9 digits: '" + tokens[0] + "'");
    }
    line.billId = tokens[0];

    if (tokens[1].empty() || tokens[1].size() > 8)
    {
        throw invalid_argument("product acronym must be 1-8 characters: '" + tokens[1] + "'");
    }
    line.productAcronym = tokens[1];

    line.unitPrice = parseNonNegative(tokens[2], "unitPriceWithoutTax");
    line.quantity = parsePositive(tokens[3], "quantity");
    line.discountTotal = parseNonNegative(tokens[4], "discountTotal");

    return line;
}