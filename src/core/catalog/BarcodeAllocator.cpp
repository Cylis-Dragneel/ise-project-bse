#include "BarcodeAllocator.h"
#include <stdexcept>
#include <set>
#include <string>

namespace {
    const int kMinSuffix = 1;
    const int kMaxSuffix = 9999;

    std::string pad4(int value) {
        std::string result = std::to_string(value);

        while (result.size() < 4) {
            result = "0" + result;
        }

        return result;
    }

    bool isValidCategoryId(const std::string& categoryId) {
        if (categoryId.size() != 2 ||
            categoryId[0] < '0' || categoryId[0] > '9' ||
            categoryId[1] < '0' || categoryId[1] > '9') {
            return false;
        }

        const int value =
            (categoryId[0] - '0') * 10 +
            (categoryId[1] - '0');

        return value >= 10 && value <= 99;
    }
}

std::string BarcodeAllocator::next(
    const std::string& categoryId,
    const std::vector<std::string>& existingBarcodes) {

    if (!isValidCategoryId(categoryId)) {
        throw std::invalid_argument(
            "category id must be between 10 and 99: " + categoryId);
    }

    std::set<int> used;

    for (const std::string& barcode : existingBarcodes) {

        if (barcode.size() != 6 ||
            barcode.substr(0, 2) != categoryId) {
            continue;
        }

        bool valid = true;

        for (std::size_t i = 2; i < 6; ++i) {
            if (barcode[i] < '0' || barcode[i] > '9') {
                valid = false;
                break;
            }
        }

        if (!valid) {
            continue;
        }

        const int suffix =
            (barcode[2] - '0') * 1000 +
            (barcode[3] - '0') * 100 +
            (barcode[4] - '0') * 10 +
            (barcode[5] - '0');

        if (suffix >= kMinSuffix &&
            suffix <= kMaxSuffix) {
            used.insert(suffix);
        }
    }

    // Find the first available barcode.
    for (int candidate = kMinSuffix;
        candidate <= kMaxSuffix;
        ++candidate) {

        if (used.count(candidate) == 0) {
            return categoryId + pad4(candidate);
        }
    }

    throw std::runtime_error(
        "barcode space for category " + categoryId + " is full");
}