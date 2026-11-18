#include "FileProductRepository.h"
#include "ProductRecordCodec.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

using std::string;
using std::vector;

namespace
{
    bool fileExists(const string& path)
    {
        std::ifstream f(path.c_str());
        return f.good();
    }
}

FileProductRepository::FileProductRepository(const string& filePath)
    : filePath_(filePath) {
}

bool FileProductRepository::renameFile(const string& from, const string& to)
{
    return std::rename(from.c_str(), to.c_str()) == 0;
}

bool FileProductRepository::removeFile(const string& path)
{
    return std::remove(path.c_str()) == 0;
}

void FileProductRepository::recoverInterruptedSave()
{
    if (!fileExists(filePath_) && fileExists(backupPath()))
    {
        if (!renameFile(backupPath(), filePath_))
            throw std::runtime_error("cannot restore " + filePath_ + " from backup " + backupPath());
    }
}

vector<Product> FileProductRepository::loadAll()
{
    recoverInterruptedSave();

    std::ifstream in(filePath_.c_str());
    if (!in)
        throw std::runtime_error("cannot open product file: " + filePath_);

    std::ostringstream buffer;
    buffer << in.rdbuf();
    if (in.bad())
        throw std::runtime_error("error while reading product file: " + filePath_);

    string content = buffer.str();
    if (content.compare(0, 3, "\xEF\xBB\xBF") == 0) // UTF-8 byte order mark added by some editors
        content.erase(0, 3);

    vector<string> records;
    try
    {
        records = ProductRecordCodec::splitRecords(content);
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
    recoverInterruptedSave();

    const string temp = tempPath();
    const string backup = backupPath();

    // 1) Write the complete new content to a temporary file.
    bool written = false;
    {
        std::ofstream out(temp.c_str(), std::ios::trunc);
        if (!out)
            throw std::runtime_error("cannot create temporary file: " + temp);

        for (size_t i = 0; i < products.size(); i++)
            out << ProductRecordCodec::format(products[i]) << "\n";

        out.close();
        written = !out.fail();
    }
    if (!written)
    {
        removeFile(temp);
        throw std::runtime_error("error while writing temporary file: " + temp +
            " (the product file was not changed)");
    }

    // 2) Move the current file out of the way (rename, never delete).
    const bool hadOriginal = fileExists(filePath_);
    if (hadOriginal)
    {
        removeFile(backup); // old backup left over from an earlier run, if any
        if (!renameFile(filePath_, backup))
        {
            removeFile(temp);
            throw std::runtime_error("cannot create backup " + backup +
                " (the product file was not changed)");
        }
    }

    // 3) Put the new file in place.
    if (!renameFile(temp, filePath_))
    {
        string message = "cannot put the new product file in place: " + filePath_;
        if (hadOriginal)
        {
            // 4b) Failure: bring the old file back.
            if (renameFile(backup, filePath_))
                message += " (the previous file was restored)";
            else
                message += " (the previous data is kept in " + backup + ")";
        }
        removeFile(temp);
        throw std::runtime_error(message);
    }

    // 4a) Success: the backup is no longer needed. Failing to delete it is harmless.
    if (hadOriginal)
        removeFile(backup);
}