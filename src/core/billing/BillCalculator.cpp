#include "BillCalculator.h"

#include "catalog/Product.h"
#include "Order.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <stdexcept>

using std::string;
using std::vector;

namespace {

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

// A bill id is exactly 9 digits; its numeric value drives the next id.
long long billIdValue(const string& billId)
{
    if (billId.size() != 9 || !isDigits(billId))
    {
        throw std::invalid_argument("invalid bill id: '" + billId + "'");
    }

    long long value = 0;
    for (char character : billId)
    {
        value = value * 10 + (character - '0');
    }

    return value;
}

} // namespace

double BillCalculator::roundMoney(double value)
{
    if (!std::isfinite(value))
    {
        throw std::invalid_argument("money value must be a finite number");
    }

    const double rounded = std::round(value * 100.0) / 100.0;

    // Do not let a tiny negative value print as "-0".
    return rounded == 0.0 ? 0.0 : rounded;
}

double BillCalculator::lineSubtotal(const BillLine& line)
{
    return roundMoney(line.unitPrice * line.quantity);
}

double BillCalculator::lineTax(const BillLine& line)
{
    return roundMoney(line.taxTotal);
}

double BillCalculator::lineDiscount(const BillLine& line)
{
    return roundMoney(line.discountTotal);
}

double BillCalculator::lineTotal(const BillLine& line)
{
    // The three components are already rounded to 2 decimals; round again so
    // floating-point addition cannot produce 1277.9999999998.
    return roundMoney(lineSubtotal(line) + lineTax(line) - lineDiscount(line));
}

BillLine BillCalculator::makeLine(const string& billId,
                                  const OrderDetail& detail,
                                  const Product& product)
{
    if (!std::isfinite(detail.quantity) || detail.quantity <= 0)
    {
        throw std::invalid_argument("quantity must be a positive number");
    }

    BillLine line;
    line.billId = billId;
    line.productAcronym = product.acronym();
    line.unitPrice = product.unitPrice();
    line.quantity = detail.quantity;
    line.discountTotal = roundMoney(product.discountPerUnit() * detail.quantity);
    line.taxTotal = roundMoney(product.taxAmount() * detail.quantity);
    line.lineTotal = lineTotal(line);
    return line;
}

void BillCalculator::calculate(Bill& bill, double seasonalPercent)
{
    if (!std::isfinite(seasonalPercent) || seasonalPercent < 0 ||
        seasonalPercent > 100)
    {
        throw std::invalid_argument(
            "seasonal discount percentage must be between 0 and 100");
    }
    if (!std::isfinite(bill.specialDiscountPercent) ||
        bill.specialDiscountPercent < 0 || bill.specialDiscountPercent > 8)
    {
        throw std::invalid_argument(
            "special discount percentage must be between 0 and 8");
    }
    if (!std::isfinite(bill.amountReceived) || bill.amountReceived < 0)
    {
        throw std::invalid_argument("amount received must be >= 0");
    }

    bill.totalNoTax = 0;
    bill.totalTax = 0;
    bill.totalDiscount = 0;
    bill.totalPayable = 0;

    for (BillLine& line : bill.lines)
    {
        line.lineTotal = lineTotal(line);
        bill.totalNoTax += lineSubtotal(line);
        bill.totalTax += lineTax(line);
        bill.totalDiscount += lineDiscount(line);
    }

    bill.totalNoTax = roundMoney(bill.totalNoTax);
    bill.totalTax = roundMoney(bill.totalTax);
    bill.totalDiscount = roundMoney(bill.totalDiscount);

    const double base = roundMoney(bill.totalNoTax + bill.totalTax -
                                   bill.totalDiscount);

    bill.seasonalDiscount = roundMoney(base * seasonalPercent / 100.0);
    const double afterSeasonal = roundMoney(base - bill.seasonalDiscount);
    const double specialAmount = roundMoney(
        afterSeasonal * bill.specialDiscountPercent / 100.0);

    bill.totalPayable = roundMoney(afterSeasonal - specialAmount);
    bill.balanceReturned = roundMoney(bill.amountReceived - bill.totalPayable);
}

string BillCalculator::nextBillId(const vector<Bill>& existing)
{
    long long maxId = 0;
    for (const Bill& bill : existing)
    {
        maxId = std::max(maxId, billIdValue(bill.billId));
    }

    const long long maxAllowed = 999999999LL;
    if (maxId >= maxAllowed)
    {
        throw std::overflow_error("bill id space exhausted");
    }

    std::ostringstream output;
    output << std::setw(9) << std::setfill('0') << (maxId + 1);
    return output.str();
}
