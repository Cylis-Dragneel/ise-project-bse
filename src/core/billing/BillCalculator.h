#ifndef BILL_CALCULATOR_H
#define BILL_CALCULATOR_H

#include <string>
#include <vector>

#include "Bill.h"

struct OrderDetail;
class Product;

class BillCalculator
{
public:
    static double roundMoney(double value);

    static double lineTotal(const BillLine& line);

    static std::string nextBillId(const std::vector<Bill>& existing);

    static BillLine makeLine(const std::string& billId,
                             const OrderDetail& detail,
                             const Product& product);

    static void calculate(Bill& bill, double amountReceived);

    static void calculate(Bill& bill,
                          const std::vector<OrderDetail>& details,
                          const std::vector<Product>& products,
                          double seasonDiscountPercent,
                          double specialDiscountPercent,
                          double amountReceived,
                          bool enforcePaymentReceived = true);
};

#endif // BILL_CALCULATOR_H