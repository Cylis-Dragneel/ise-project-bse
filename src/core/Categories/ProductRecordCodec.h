#pragma once
#ifndef PRODUCT_RECORD_CODEC_H
#define PRODUCT_RECORD_CODEC_H

#include <string>
#include <vector>
#include "Product.h"

class ProductRecordCodec
{
public:
    static std::vector<std::string> splitRecords(const std::string& fileContent);
    static Product parse(const std::string& record);
    static std::string format(const Product& product);
};

#endif