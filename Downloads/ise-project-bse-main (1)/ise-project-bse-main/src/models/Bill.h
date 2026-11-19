#ifndef BILL_H
#define BILL_H

#include <string>
#include <sstream>
#include <iomanip>

struct Bill {
    std::string billId;      // 9-digit
    std::string customerName;
    std::string orderId;
    std::string productAcronym;
    double unitPriceNoTax = 0;
    double discountPerUnit = 0;
    int quantity = 0;
    double totalNoTax = 0;
    double totalTax = 0;
    double totalDiscount = 0;
    double totalPayable = 0;
    std::string paymentStatus;

    std::string toLine() const {
        std::ostringstream o;
        o << billId << ' ' << customerName << ' ' << orderId << ' '
          << productAcronym << ' '
          << std::fixed << std::setprecision(2)
          << unitPriceNoTax << ' ' << discountPerUnit << ' '
          << quantity << ' ' << totalNoTax << ' '
          << totalTax << ' ' << totalDiscount << ' '
          << totalPayable << ' ' << paymentStatus << " #";
        return o.str();
    }
};

#endif