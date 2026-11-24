#include "BarcodeAllocator.h"
#include <stdexcept>
#include <set>

namespace {
    const int kMinSuffix = 1;
    const int kMaxSuffix = 9999;
}

std::string BarcodeAllocator::next(const std::string& categoryId,
    const std::vector<std::string>& existingBarcodes) {
    if (categoryId.size() != 2 ||
        categoryId[0] < '0' || categoryId[0] > '9' ||
        categoryId[1] < '0' || categoryId[1] > '9') {
        throw std::invalid_argument("category id must be 2 digits: " + categoryId);
    }
    std::set<int> used;
    int maxUsed = 0;
    for (const std::string& barcode : existingBarcodes) {
        if (barcode.size() != 6) continue;
        if (barcode.substr(0, 2) != categoryId) continue;
        bool digits = true;
        for (std::size_t i = 2; i < 6; ++i) {
            if (barcode[i] < '0' || barcode[i] > '9') { digits = false; break; }
        }
        if (!digits) continue;
        const int suffix = (barcode[2] - '0') * 1000 + (barcode[3] - '0') * 100
            + (barcode[4] - '0') * 10 + (barcode[5] - '0');
        used.insert(suffix);
        if (suffix > maxUsed) maxUsed = suffix;
    }
    if (maxUsed < kMaxSuffix) {
        return categoryId + pad4(maxUsed + 1);
    }
    for (int candidate = kMinSuffix; candidate <= kMaxSuffix; ++candidate) {
        if (used.count(candidate) == 0) {
            return categoryId + pad4(candidate);
        }
    }
    throw std::runtime_error("barcode space for category " + categoryId + " is full");
}

// helper — put in anonymous namespace at top
static std::string pad4(int n) {
    std::string s = std::to_string(n);
    while (s.size() < 4) s = "0" + s;
    return s;
}