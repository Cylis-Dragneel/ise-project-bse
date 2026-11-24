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

    static std::string nextBillId(const std::vector<Bill>& existing);

    static void calculate(Bill& bill,
                          const std::vector<OrderDetail>& details,
                          const std::vector<Product>& products,
                          double seasonDiscountPercent,
                          double specialDiscountPercent,
                          double amountReceived,
                          bool enforcePaymentReceived = true);
};

#endif // BILL_CALCULATOR_H