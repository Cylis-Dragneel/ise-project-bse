#pragma once
#ifndef FILE_PRODUCT_REPOSITORY_H
#define FILE_PRODUCT_REPOSITORY_H

#include <string>
#include "IProductRepository.h"

// Stores products in a text file (Product.txt). Only does file I/O;
// the format itself is handled by ProductRecordCodec.
class FileProductRepository : public IProductRepository
{
private:
    std::string filePath_;

public:
    explicit FileProductRepository(const std::string& filePath);

    // Throws std::runtime_error (cannot open file) or
    // std::invalid_argument (bad record, message names the record number).
    std::vector<Product> loadAll() override;

    // Writes to a temporary file first so a failure cannot destroy Product.txt.
    void saveAll(const std::vector<Product>& products) override;
};

#endif