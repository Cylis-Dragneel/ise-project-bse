#include "BillManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

// Absolute path to match OrderManager and ProductManager
static const std::string BILL_FILE_PATH = "Bill.txt";
BillManager::BillManager(const std::string& path) : filePath(BILL_FILE_PATH) {}

std::string BillManager::generateBillId() const {
    int maxId = 0;
    std::ifstream f(filePath);
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream in(line);
        std::string id;
        if (in >> id) {
            try { 
                maxId = std::max(maxId, std::stoi(id)); 
            } catch (...) {}
        }
    }
    std::ostringstream o;
    o << std::setw(9) << std::setfill('0') << (maxId + 1);
    return o.str();
}

bool BillManager::appendBills(const std::vector<Bill>& bills) {
    std::ofstream f(filePath, std::ios::app);
    if (!f.is_open()) return false;
    for (const auto& b : bills) f << b.toLine() << '\n';
    return f.good();
}