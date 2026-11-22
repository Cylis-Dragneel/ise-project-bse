#ifndef BILL_H
#define BILL_H

#include <string>
#include <vector>

// BillDetail.txt record (one line per item):
//   billId productAcronym unitPriceWithoutTax quantity discountTotal #
//
// The bill id links to Bill.txt. The product acronym links to Product.txt.
// discountTotal is the whole-line discount (per-unit discount * quantity), as
// required by the increment-3 specification.
//
// taxTotal and lineTotal are transient: they are filled when a line is built
// from a product and are not written to BillDetail.txt. A bill loaded from disk
// therefore has taxTotal = 0; its header still carries the total tax.
struct BillLine {
    std::string billId;          // 9 digits
    std::string productAcronym;  // e.g. MILK1L
    double unitPrice = 0;        // without tax
    double quantity = 0;
    double discountTotal = 0;    // whole-line discount

    double taxTotal = 0;         // transient, whole line
    double lineTotal = 0;        // transient, unitPrice*qty + tax - discount
};

// Bill.txt record (one line per bill):
//   billId customerName orderId totalNoTax totalTax totalDiscount totalPayable
//   amountReceived balanceReturned paymentStatus seasonalDiscount
//   specialDiscount billDate billTime #
//
// seasonalDiscount is an amount in PKR. specialDiscount is a percentage (0-8),
// as the specification says "not more than 8 percent"; the amount is derived.
struct Bill {
    std::string billId;        // 9 digits, unique in Bill.txt
    std::string customerName;  // empty is written as "-"
    std::string orderId;       // 8 digits, links to Order.txt

    double totalNoTax = 0;
    double totalTax = 0;
    double totalDiscount = 0;  // product-level discounts only
    double totalPayable = 0;
    double amountReceived = 0;
    double balanceReturned = 0;

    std::string paymentStatus; // in-progress, on-hold, paid, archived
    double seasonalDiscount = 0;
    double specialDiscountPercent = 0; // 0-8
    std::string billDate;      // YYYY-MM-DD
    std::string billTime;      // HH:MM:SS

    std::vector<BillLine> lines;
};

#endif // BILL_H
