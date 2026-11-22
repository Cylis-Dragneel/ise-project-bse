#ifndef BILL_CALCULATOR_H
#define BILL_CALCULATOR_H

#include <string>
#include <vector>

#include "Bill.h"

struct OrderDetail;
class Product;

// All billing arithmetic and the bill-id rule live here, away from files and
// the GUI, so every screen shows the same numbers.
//
// Rounding rule: money is rounded to 2 decimal places per line component
// (subtotal, tax, discount) and then added. A bill's totals are the sums of
// the rounded line values, so the printed lines always add up to the total.
//
// Discount order (confirmed): seasonal discount first, then the special
// discount on the already reduced base.
class BillCalculator
{
public:
    // Rounds to the nearest paisa (2 decimals), half away from zero.
    static double roundMoney(double value);

    static double lineSubtotal(const BillLine& line);  // unitPrice * quantity
    static double lineTax(const BillLine& line);       // transient taxTotal
    static double lineDiscount(const BillLine& line);  // discountTotal
    static double lineTotal(const BillLine& line);     // subtotal + tax - discount

    // Builds one line from an order row and the catalog's product. The product
    // is the source of truth for price, tax and discount.
    static BillLine makeLine(const std::string& billId,
                             const OrderDetail& detail,
                             const Product& product);

    // Fills lineTotal for every line and all bill totals. `seasonalPercent` is
    // the percentage of the active season for the bill date (0 when none).
    // The special percentage is read from the bill (0-8).
    static void calculate(Bill& bill, double seasonalPercent = 0);

    // Highest existing id + 1, zero-padded to 9 digits. The first bill of an
    // empty file is "000000001". Throws std::invalid_argument for a malformed
    // id and std::overflow_error when 999999999 has been used.
    static std::string nextBillId(const std::vector<Bill>& existing);
};

#endif // BILL_CALCULATOR_H
