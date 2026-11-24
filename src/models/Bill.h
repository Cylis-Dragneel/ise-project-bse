#ifndef BILL_H
#define BILL_H

#include <string>
#include <vector>

// ============================================================================
// INCREMENT 2 MODELS (Preserved for backward compatibility & existing tests)
// ============================================================================

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

// ============================================================================
// INCREMENT 3 MODELS (Redesigned 14-field Bill.txt & 5-field BillDetail.txt)
// ============================================================================

namespace inc3 {

// BillDetail.txt record (one line per item, space-separated, '#' at end):
//   billId productAcronym unitPriceWithoutTax quantity discountTotal #
struct BillDetailLine {
    std::string billId;         // 9 digits, e.g. "000000001"
    std::string productAcronym; // unique product acronym (max 8 chars)
    double unitPrice = 0.0;     // unit price without tax
    double quantity = 0.0;      // count or weight
    double discountTotal = 0.0; // whole-line discount (discountPerUnit * quantity)
};

// Bill.txt record (one line per bill, 14 fields + '#' at end):
//   billId customerName orderId totalNoTax totalTax totalDiscount totalPayable
//   amountReceived balanceReturned paymentStatus seasonalDiscount
//   specialDiscountPercent billDate billTime #
struct BillHeader {
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

    std::vector<BillDetailLine> lines;   // joined from BillDetail.txt by billId
};

// Interface implemented by Person E's SeasonService (or injected in tests)
class ISeasonProvider {
public:
    virtual ~ISeasonProvider() = default;
    // Returns active discount percentage (0-100) for YYYY-MM-DD, or 0 if none applies.
    virtual double activePercent(const std::string& date) const = 0;
};

} // namespace inc3

#endif // BILL_H
