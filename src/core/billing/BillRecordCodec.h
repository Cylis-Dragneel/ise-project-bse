#ifndef BILL_RECORD_CODEC_H
#define BILL_RECORD_CODEC_H

#include <string>
#include <vector>

#include "Bill.h"

// Text form of one Bill.txt line (increment 3, one line per bill):
//
//   billId customerName orderId totalNoTax totalTax totalDiscount totalPayable
//   amountReceived balanceReturned paymentStatus seasonalDiscount
//   specialDiscount billDate billTime #
//
// Kept separate from the repository so the format can be tested without
// touching the disk.
class BillRecordCodec
{
public:
    // Parses ONE record. A trailing '#' is optional, so the output of format()
    // can be parsed directly. Throws std::invalid_argument.
    static Bill parse(const std::string& record);

    // Always ends with '#'.
    static std::string format(const Bill& bill);

    // Splits a whole file into records (text before each '#'). Throws if the
    // last record has no '#' marker.
    static std::vector<std::string> splitRecords(const std::string& content);
};

#endif // BILL_RECORD_CODEC_H
