#ifndef BILL_H
#define BILL_H

#include <string>
#include <vector>

// BillDetail.txt record (5 fields + '#' at end):
//   billId productAcronym unitPriceWithoutTax quantity discountTotal #
struct BillLine {
    std::string billId;         // 9 digits, e.g. "000000001"
    std::string productAcronym; // unique product acronym (max 8 chars)
    double unitPrice = 0.0;     // unit price without tax
    double quantity = 0.0;      // count or weight
    double discountTotal = 0.0; // whole-line discount (discountPerUnit * quantity)
};

// Bill.txt record (14 fields + '#' at end):
//   billId customerName orderId totalNoTax totalTax totalDiscount totalPayable
//   amountReceived balanceReturned paymentStatus seasonalDiscount
//   specialDiscountPercent billDate billTime #
struct Bill {
    std::string billId;                  // 9 digits, unique in Bill.txt
    std::string customerName;            // sanitized ('_' for spaces, "-" when empty)
    std::string orderId;                 // 8 digits from Order.txt
    double totalNoTax = 0.0;
    double totalTax = 0.0;
    double totalDiscount = 0.0;          // sum of product-level discounts only
    double totalPayable = 0.0;
    double amountReceived = 0.0;
    double balanceReturned = 0.0;
    std::string paymentStatus;           // copied from Order.txt
    double seasonalDiscount = 0.0;       // amount in PKR (0 when no season applies)
    double specialDiscountPercent = 0.0; // percentage 0..8
    std::string billDate;                // YYYY-MM-DD
    std::string billTime;                // HH:MM:SS

    std::vector<BillLine> lines;         // joined from BillDetail.txt by billId
};

#endif // BILL_H