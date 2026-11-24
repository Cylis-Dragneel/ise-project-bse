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

// ============================================================================
// INCREMENT 2 IMPLEMENTATION
// ============================================================================

double BillCalculator::roundMoney(double value)
{
    if (!std::isfinite(value))
    {
        throw std::invalid_argument("money value must be a finite number");
    }
    const double rounded = std::round(value * 100.0) / 100.0;
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
                throw std::invalid_argument("bill " + line.billId + " mixes order ids");
            }
            if (bill.paymentStatus != line.paymentStatus)
            {
                throw std::invalid_argument("bill " + line.billId + " mixes payment statuses");
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

// ============================================================================
// INCREMENT 3 IMPLEMENTATION
// ============================================================================

namespace inc3 {

double BillingCalculator::roundMoney(double value)
{
    return BillCalculator::roundMoney(value);
}

string BillingCalculator::nextBillId(const vector<BillHeader>& existing)
{
    long long maxId = 0;
    for (const BillHeader& bill : existing)
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

BillDetailLine BillingCalculator::makeDetailLine(const string& billId,
                                                 const OrderDetail& detail,
                                                 const Product& product)
{
    BillDetailLine line;
    line.billId = billId;
    line.productAcronym = product.acronym();
    line.unitPrice = roundMoney(product.unitPrice());
    line.quantity = detail.quantity;
    line.discountTotal = roundMoney(product.discountPerUnit() * detail.quantity);
    return line;
}

void BillingCalculator::calculate(BillHeader& bill,
                                  const vector<OrderDetail>& details,
                                  const vector<Product>& products,
                                  double seasonDiscountPercent,
                                  double specialDiscountPercent,
                                  double amountReceived)
{
    if (details.size() != products.size())
    {
        throw std::invalid_argument("details and products size mismatch");
    }
    if (!std::isfinite(seasonDiscountPercent) || seasonDiscountPercent < 0.0 || seasonDiscountPercent > 100.0)
    {
        throw std::invalid_argument("seasonal discount percent must be between 0 and 100");
    }
    if (!std::isfinite(specialDiscountPercent) || specialDiscountPercent < 0.0 || specialDiscountPercent > 8.0)
    {
        throw std::invalid_argument("special discount percent must be between 0 and 8");
    }
    if (!std::isfinite(amountReceived) || amountReceived < 0.0)
    {
        throw std::invalid_argument("amount received must be >= 0");
    }

    bill.lines.clear();
    bill.lines.reserve(details.size());

    double sumNoTax = 0.0;
    double sumTax = 0.0;
    double sumDiscount = 0.0;

    for (std::size_t i = 0; i < details.size(); ++i)
    {
        const OrderDetail& detail = details[i];
        const Product& product = products[i];

        const double lineSub = roundMoney(product.unitPrice() * detail.quantity);
        const double lineTax = roundMoney(product.taxAmount() * detail.quantity);
        const double lineDisc = roundMoney(product.discountPerUnit() * detail.quantity);

        sumNoTax = roundMoney(sumNoTax + lineSub);
        sumTax = roundMoney(sumTax + lineTax);
        sumDiscount = roundMoney(sumDiscount + lineDisc);

        BillDetailLine line;
        line.billId = bill.billId;
        line.productAcronym = product.acronym();
        line.unitPrice = roundMoney(product.unitPrice());
        line.quantity = detail.quantity;
        line.discountTotal = lineDisc;
        bill.lines.push_back(std::move(line));
    }

    bill.totalNoTax = sumNoTax;
    bill.totalTax = sumTax;
    bill.totalDiscount = sumDiscount;

    // Step 3: base = totalNoTax + totalTax - totalDiscount
    double base = roundMoney(bill.totalNoTax + bill.totalTax - bill.totalDiscount);
    if (base < 0.0)
    {
        base = 0.0;
    }

    // Step 4: seasonalDiscount = round(base * seasonPercent / 100)
    bill.seasonalDiscount = (seasonDiscountPercent > 0.0)
                                ? roundMoney(base * seasonDiscountPercent / 100.0)
                                : 0.0;

    // Step 5: specialDiscountAmount = round((base - seasonalDiscount) * specialPercent / 100)
    double afterSeasonal = roundMoney(base - bill.seasonalDiscount);
    if (afterSeasonal < 0.0)
    {
        afterSeasonal = 0.0;
    }
    bill.specialDiscountPercent = roundMoney(specialDiscountPercent);
    const double specialDiscountAmount =
        roundMoney(afterSeasonal * bill.specialDiscountPercent / 100.0);

    // Step 6: totalPayable = base - seasonalDiscount - specialDiscountAmount
    bill.totalPayable = roundMoney(base - bill.seasonalDiscount - specialDiscountAmount);
    if (bill.totalPayable < 0.0)
    {
        bill.totalPayable = 0.0;
    }

    // Step 7: balanceReturned = amountReceived - totalPayable (must be >= 0)
    bill.amountReceived = roundMoney(amountReceived);
    const double balance = roundMoney(bill.amountReceived - bill.totalPayable);
    if (balance < 0.0)
    {
        if (bill.paymentStatus == "paid")
        {
            throw std::invalid_argument("amount received is less than total payable");
        }
        bill.balanceReturned = 0.0;
    }
    else
    {
        bill.balanceReturned = balance;
    }
}

} // namespace inc3
