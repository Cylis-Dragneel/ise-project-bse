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
class BillCalculator
{
public:
    // Rounds to the nearest paisa (2 decimals), half away from zero.
    static double roundMoney(double value);

    static double lineSubtotal(const BillLine& line);  // unitPrice * quantity
    static double lineTax(const BillLine& line);       // taxPerUnit * quantity
    static double lineDiscount(const BillLine& line);  // discountPerUnit * quantity
    static double lineTotal(const BillLine& line);     // subtotal + tax - discount

    // Builds one line from an order row and the catalog's product. The product
    // is the source of truth for price, tax and discount.
    static BillLine makeLine(const std::string& billId,
                             const std::string& orderId,
                             const OrderDetail& detail,
                             const Product& product,
                             const std::string& paymentStatus);

    // Fills lineTotal for every line and the four bill totals.
    static void calculate(Bill& bill);

    // Groups flat Bill.txt lines into bills, preserving the order in which the
    // bill ids first appear. Calls calculate() on every bill.
    static std::vector<Bill> group(const std::vector<BillLine>& lines);

    // Highest existing id + 1, zero-padded to 9 digits. The first bill of an
    // empty file is "000000001". Throws std::invalid_argument for a malformed
    // id and std::overflow_error when 999999999 has been used.
    static std::string nextBillId(const std::vector<BillLine>& existing);
};

#endif // BILL_CALCULATOR_H
