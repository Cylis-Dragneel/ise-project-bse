#ifndef BILL_H
#define BILL_H

#include <string>
#include <vector>

// Bill.txt record (one line per bill item, all bills of one order share the
// bill id):
//   billId orderId productId productBarcode quantity unitPrice
//   taxPerUnit discountPerUnit lineTotal paymentStatus #
//
// All fields are single tokens, so a record can be split on whitespace. The
// customer name and the product acronym are deliberately not copied here:
// they live in Order.txt / Product.txt and are joined through orderId and
// productId. This keeps names with spaces out of the bill file.
//
// Money is in PKR. unitPrice, taxPerUnit and discountPerUnit are per unit;
// lineTotal is the rounded amount actually charged for the line.
struct BillLine {
    std::string billId;        // 9 digits, e.g. "000000001"
    std::string orderId;       // 8 digits, links to Order.txt
    int productId = 0;
    std::string productBarcode;
    double quantity = 0;       // double for weights like 4.5 kg
    double unitPrice = 0;      // before tax and discount
    double taxPerUnit = 0;
    double discountPerUnit = 0;
    double lineTotal = 0;      // (unitPrice + taxPerUnit - discountPerUnit) * quantity
    std::string paymentStatus; // in-progress, on-hold, paid, archived
};

// One bill (receipt) for one order. The line totals and the bill totals are
// derived values; BillCalculator fills them in.
struct Bill {
    std::string billId;
    std::string orderId;
    std::string paymentStatus;
    std::vector<BillLine> lines;

    double totalNoTax = 0;
    double totalTax = 0;
    double totalDiscount = 0;
    double totalPayable = 0;
};

#endif // BILL_H
