#include "FileProductRepository.h"
#include "ProductRecordCodec.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

using std::string;
using std::vector;

FileProductRepository::FileProductRepository(const string& filePath)
    : filePath_(filePath) {
}

vector<Product> FileProductRepository::loadAll()
{
    std::ifstream in(filePath_.c_str());
    if (!in)
        throw std::runtime_error("cannot open product file: " + filePath_);

    std::ostringstream buffer;
    buffer << in.rdbuf();

    vector<string> records;
    try
    {
        records = ProductRecordCodec::splitRecords(buffer.str());
    }
    catch (const std::invalid_argument& e)
    {
        throw std::invalid_argument(filePath_ + ": " + e.what());
    }

    vector<Product> products;
    for (size_t i = 0; i < records.size(); i++)
    {
        try
        {
            products.push_back(ProductRecordCodec::parse(records[i]));
        }
        catch (const std::invalid_argument& e)
        {
            throw std::invalid_argument(filePath_ + ", record " + std::to_string(i + 1) +
                ": " + e.what());
        }
    }
    return products;
}

void FileProductRepository::saveAll(const vector<Product>& products)
{
    const string tempPath = filePath_ + ".tmp";
    {
        std::ofstream out(tempPath.c_str(), std::ios::trunc);
        if (!out)
            throw std::runtime_error("cannot write temporary file: " + tempPath);

        for (size_t i = 0; i < products.size(); i++)
            out << ProductRecordCodec::format(products[i]) << "\n";

        out.flush();
        if (!out)
            throw std::runtime_error("error while writing: " + tempPath);
    }

    std::remove(filePath_.c_str()); // needed on Windows, harmless elsewhere
    if (std::rename(tempPath.c_str(), filePath_.c_str()) != 0)
        throw std::runtime_error("cannot replace product file: " + filePath_);
}