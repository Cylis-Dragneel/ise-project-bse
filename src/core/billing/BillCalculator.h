#ifndef BILL_CALCULATOR_H
#define BILL_CALCULATOR_H

#include <string>
#include <vector>

#include "Bill.h"

struct OrderDetail;
class Product;

// ============================================================================
// INCREMENT 2 CALCULATOR (Preserved)
// ============================================================================
class BillCalculator
{
public:
    static double roundMoney(double value);

    static double lineSubtotal(const BillLine& line);  // unitPrice * quantity
    static double lineTax(const BillLine& line);       // taxPerUnit * quantity
    static double lineDiscount(const BillLine& line);  // discountPerUnit * quantity
    static double lineTotal(const BillLine& line);     // subtotal + tax - discount

    static BillLine makeLine(const std::string& billId,
                             const std::string& orderId,
                             const OrderDetail& detail,
                             const Product& product,
                             const std::string& paymentStatus);

    static void calculate(Bill& bill);
    static std::vector<Bill> group(const std::vector<BillLine>& lines);
    static std::string nextBillId(const std::vector<BillLine>& existing);
};

// ============================================================================
// INCREMENT 3 CALCULATOR (Seasonal + Special 0-8% + Amount Received/Balance)
// ============================================================================
namespace inc3 {

class BillingCalculator
{
public:
    static double roundMoney(double value);

    // Highest existing billId + 1, zero-padded to 9 digits ("000000001" if empty).
    static std::string nextBillId(const std::vector<BillHeader>& existing);

    // Builds one BillDetailLine from an OrderDetail and catalog Product.
    static BillDetailLine makeDetailLine(const std::string& billId,
                                         const OrderDetail& detail,
                                         const Product& product);

    // Computes all totals, seasonal discount (applied first on base),
    // special discount (0-8%, applied after seasonal), totalPayable, and balanceReturned.
    static void calculate(BillHeader& bill,
                          const std::vector<OrderDetail>& details,
                          const std::vector<Product>& products,
                          double seasonDiscountPercent,
                          double specialDiscountPercent,
                          double amountReceived);
};

} // namespace inc3

#endif // BILL_CALCULATOR_H
