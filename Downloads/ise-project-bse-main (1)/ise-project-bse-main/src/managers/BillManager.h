#ifndef BILLMANAGER_H
#define BILLMANAGER_H

#include <string>
#include <vector>
#include "../models/Bill.h"

class BillManager {
public:
    explicit BillManager(const std::string& path = "Bill.txt");

    // Generates a 9-digit bill id (sequential).
    std::string generateBillId() const;

    // Appends bills to Bill.txt.
    bool appendBills(const std::vector<Bill>& bills);

private:
    std::string filePath;
};

#endif