#pragma once
#ifndef PRODUCT_RECORD_CODEC_H
#define PRODUCT_RECORD_CODEC_H

#include <string>
#include <vector>
#include "Product.h"

// Knows ONLY the text format of Product.txt. No file access here.
//
// One record:
//   id barcode acronym name unit unitPrice tax discount quantity barcodePath#
// Name may be written with '_' instead of spaces (Milk_1_Litre); both forms
// are accepted when reading, '_' is used when writing.
class ProductRecordCodec
{
public:
    // Splits the whole file content into records (text before each '#').
    // Throws std::invalid_argument if the last record has no '#'.
    static std::vector<std::string> splitRecords(const std::string& fileContent);

    // Parses ONE record (without the '#'). Throws std::invalid_argument.
    static Product parse(const std::string& record);

    // One record including the '#' marker (no newline).
    static std::string format(const Product& product);
};

#endif