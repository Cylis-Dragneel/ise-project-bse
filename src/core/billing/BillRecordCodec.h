#ifndef BILL_RECORD_CODEC_H
#define BILL_RECORD_CODEC_H

#include <string>
#include <vector>

#include "Bill.h"

// ============================================================================
// INCREMENT 2 CODEC (Preserved)
// ============================================================================
class BillRecordCodec
{
public:
    static BillLine parse(const std::string& record);
    static std::string format(const BillLine& line);
    static std::vector<std::string> splitRecords(const std::string& content);
};

// ============================================================================
// INCREMENT 3 CODECS (14-field Bill.txt & 5-field BillDetail.txt)
// ============================================================================
namespace inc3 {

class BillHeaderCodec
{
public:
    // Sanitizes customer name: replaces whitespace with '_', returns "-" if empty.
    static std::string sanitizeCustomerName(const std::string& rawName);

    // Formats all 14 space-separated fields followed by " #".
    static std::string format(const BillHeader& bill);

    // Parses one record (trailing '#' optional). Throws std::invalid_argument on error.
    static BillHeader parse(const std::string& record);

    // Splits file content into '#'-terminated records. Throws if final record lacks '#'.
    static std::vector<std::string> splitRecords(const std::string& content);
};

class BillDetailCodec
{
public:
    // Formats all 5 space-separated fields followed by " #".
    static std::string format(const BillDetailLine& line);

    // Parses one record (trailing '#' optional). Throws std::invalid_argument on error.
    static BillDetailLine parse(const std::string& record);

    // Splits file content into '#'-terminated records. Throws if final record lacks '#'.
    static std::vector<std::string> splitRecords(const std::string& content);
};

} // namespace inc3

#endif // BILL_RECORD_CODEC_H
