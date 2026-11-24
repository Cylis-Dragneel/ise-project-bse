#ifndef BILL_RECORD_CODEC_H
#define BILL_RECORD_CODEC_H

#include <string>
#include <vector>

#include "Bill.h"

// 14-field Bill.txt record codec
class BillRecordCodec
{
public:
    static std::string sanitizeCustomerName(const std::string& rawName);
    static std::string format(const Bill& bill);
    static Bill parse(const std::string& record);
    static std::vector<std::string> splitRecords(const std::string& content);
};

// 5-field BillDetail.txt record codec
class BillDetailRecordCodec
{
public:
    static std::string format(const BillLine& line);
    static BillLine parse(const std::string& record);
    static std::vector<std::string> splitRecords(const std::string& content);
};

#endif // BILL_RECORD_CODEC_H