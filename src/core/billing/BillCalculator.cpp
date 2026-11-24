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
    return rounded == 0.0 ? 0.0 : rounded;
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

BillLine BillCalculator::makeLine(const string& billId,
                                  const OrderDetail& detail,
                                  const Product& product)
{
    BillLine line;
    line.billId = billId;
    line.productAcronym = product.acronym();
    line.unitPrice = roundMoney(product.unitPrice());
    line.quantity = detail.quantity;
    line.discountTotal = roundMoney(product.discountPerUnit() * detail.quantity);
    const double lineSub = roundMoney(product.unitPrice() * detail.quantity);
    const double lineTax = roundMoney(product.taxAmount() * detail.quantity);
    line.lineTotal = roundMoney(lineSub + lineTax - line.discountTotal);
    return line;
}

void BillCalculator::calculate(Bill& bill, double amountReceived)
{
    if (!std::isfinite(amountReceived) || amountReceived < 0.0)
    {
        throw std::invalid_argument("amount received must be >= 0");
    }

    double sumNoTax = 0.0;
    double sumDiscount = 0.0;
    double sumLines = 0.0;

    for (BillLine& line : bill.lines)
    {
        const double lineSub = roundMoney(line.unitPrice * line.quantity);
        if (line.lineTotal == 0.0 && (lineSub > 0.0 || line.discountTotal > 0.0))
        {
            line.lineTotal = roundMoney(lineSub - line.discountTotal);
        }
        sumNoTax = roundMoney(sumNoTax + lineSub);
        sumDiscount = roundMoney(sumDiscount + line.discountTotal);
        sumLines = roundMoney(sumLines + line.lineTotal);
    }

    bill.totalNoTax = sumNoTax;
    bill.totalDiscount = sumDiscount;
    bill.totalPayable = sumLines < 0.0 ? 0.0 : sumLines;
    bill.amountReceived = roundMoney(amountReceived);
    const double balance = roundMoney(bill.amountReceived - bill.totalPayable);
    bill.balanceReturned = balance < 0.0 ? 0.0 : balance;
}

void BillCalculator::calculate(Bill& bill,
                                 const vector<OrderDetail>& details,
                                 const vector<Product>& products,
                                 double seasonDiscountPercent,
                                 double specialDiscountPercent,
                                 double amountReceived,
                                 bool enforcePaymentReceived)
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

        BillLine line;
        line.billId = bill.billId;
        line.productAcronym = product.acronym();
        line.unitPrice = roundMoney(product.unitPrice());
        line.quantity = detail.quantity;
        line.discountTotal = lineDisc;
        line.lineTotal = roundMoney(lineSub + lineTax - lineDisc);
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
        if (enforcePaymentReceived && bill.paymentStatus == "paid")
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