#include "BillCalculator.h"

#include "catalog/Product.h"
#include "Order.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <map>
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
    return roundMoney(line.taxPerUnit * line.quantity);
}

double BillCalculator::lineDiscount(const BillLine& line)
{
    return roundMoney(line.discountPerUnit * line.quantity);
}

double BillCalculator::lineTotal(const BillLine& line)
{
    // The three components are already rounded to 2 decimals; round again so
    // floating-point addition cannot produce 1277.9999999998.
    return roundMoney(lineSubtotal(line) + lineTax(line) - lineDiscount(line));
}

BillLine BillCalculator::makeLine(const string& billId,
                                  const string& orderId,
                                  const OrderDetail& detail,
                                  const Product& product,
                                  const string& paymentStatus)
{
    BillLine line;
    line.billId = billId;
    line.orderId = orderId;
    line.productId = product.id();
    line.productBarcode = product.barcode();
    line.quantity = detail.quantity;
    line.unitPrice = product.unitPrice();
    line.taxPerUnit = product.taxAmount();
    line.discountPerUnit = product.discountPerUnit();
    line.paymentStatus = paymentStatus;
    line.lineTotal = lineTotal(line);
    return line;
}

void BillCalculator::calculate(Bill& bill)
{
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
        bill.totalPayable += line.lineTotal;
    }

    bill.totalNoTax = roundMoney(bill.totalNoTax);
    bill.totalTax = roundMoney(bill.totalTax);
    bill.totalDiscount = roundMoney(bill.totalDiscount);
    bill.totalPayable = roundMoney(bill.totalPayable);
}

vector<Bill> BillCalculator::group(const vector<BillLine>& lines)
{
    vector<Bill> bills;
    std::map<string, std::size_t> indexById;

    for (const BillLine& line : lines)
    {
        auto found = indexById.find(line.billId);
        if (found == indexById.end())
        {
            Bill bill;
            bill.billId = line.billId;
            bill.orderId = line.orderId;
            bill.paymentStatus = line.paymentStatus;
            bill.lines.push_back(line);

            indexById[line.billId] = bills.size();
            bills.push_back(bill);
        }
        else
        {
            Bill& bill = bills[found->second];
            if (bill.orderId != line.orderId)
            {
                throw std::invalid_argument("bill " + line.billId +
                                            " mixes order ids");
            }
            if (bill.paymentStatus != line.paymentStatus)
            {
                throw std::invalid_argument("bill " + line.billId +
                                            " mixes payment statuses");
            }
            bill.lines.push_back(line);
        }
    }

    for (Bill& bill : bills)
    {
        calculate(bill);
    }

    return bills;
}

string BillCalculator::nextBillId(const vector<BillLine>& existing)
{
    long long maxId = 0;
    for (const BillLine& line : existing)
    {
        maxId = std::max(maxId, billIdValue(line.billId));
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
