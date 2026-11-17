
#ifndef BILL_H
#define BILL_H

#include <string>
#include <vector>

struct Bill
{
    std::string billId;
    std::string customerName;
    std::string orderId;

    double totalWithoutTax = 0.0;
    double totalTax = 0.0;
    double totalDiscount = 0.0;
    double totalPayable = 0.0;

    std::string paymentStatus;
};

#endif
