#pragma once
#ifndef BARCODEALLOCATOR_H
#define BARCODEALLOCATOR_H

#include <string>
#include <vector>

class BarcodeAllocator {
public:
    static std::string next(const std::string& categoryId,
        const std::vector<std::string>& existingBarcodes);
};

#endif