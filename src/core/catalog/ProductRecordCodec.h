#pragma once
#ifndef PRODUCT_RECORD_CODEC_H
#define PRODUCT_RECORD_CODEC_H

#include <string>
#include <vector>
#include "Product.h"

// Knows ONLY the text format of Product.txt. No file access here.
//
// One record (increment 3):
//   id barcode categoryId acronym name unit unitPrice tax discount quantity barcodePath#
// Name may be written with '_' instead of spaces (Milk_1_Litre); both forms
// are accepted when reading, '_' is used when writing.
//
// Legacy increment-2 records have no categoryId:
//   id barcode acronym name unit unitPrice tax discount quantity barcodePath#
// They are still accepted; the category id is inferred from the first two
// barcode digits. format() always writes the 11-field form.
class ProductRecordCodec
{
public:
    // Splits the whole file content into records (text before each '#').
    // Throws std::invalid_argument if the last record has no '#'.
    static std::vector<std::string> splitRecords(const std::string& fileContent);

    // Parses ONE record (without the '#'). A trailing '#' is also accepted, so
    // the output of format() can be parsed directly. Throws std::invalid_argument.
    static Product parse(const std::string& record);

    // One record including the '#' marker (no newline).
    static std::string format(const Product& product);
};

#endif