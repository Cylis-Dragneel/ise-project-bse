#include "FileCategoryRepository.h"
#include "CategoryRecordCodec.h"

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

FileCategoryRepository::FileCategoryRepository(const string& filePath)
    : filePath_(filePath)
{
}

bool FileCategoryRepository::renameFile(const string& from, const string& to)
{
    return std::rename(from.c_str(), to.c_str()) == 0;
}

bool FileCategoryRepository::removeFile(const string& path)
{
    return std::remove(path.c_str()) == 0;
}

void FileCategoryRepository::recoverInterruptedSave()
{
    if (!fileExists(filePath_) && fileExists(backupPath()))
    {
        if (!renameFile(backupPath(), filePath_))
            throw std::runtime_error("cannot restore " + filePath_ + " from backup " + backupPath());

        if (fileExists(tempPath()))
            removeFile(tempPath());
    }
}

vector<ProductCategory> FileCategoryRepository::loadAll()
{
    recoverInterruptedSave();

    std::ifstream input(filePath_.c_str());
    if (!input)
    {
        // PCategory.txt does not exist until the first category is added.
        return vector<ProductCategory>();
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad())
        throw std::runtime_error("error while reading category file: " + filePath_);

    string content = buffer.str();
    if (content.compare(0, 3, "\xEF\xBB\xBF") == 0) // UTF-8 BOM added by some editors
        content.erase(0, 3);

    vector<string> records;
    try
    {
        records = CategoryRecordCodec::splitRecords(content);
    }
    catch (const std::invalid_argument& error)
    {
        throw std::invalid_argument(filePath_ + ": " + error.what());
    }

    vector<ProductCategory> categories;
    for (size_t i = 0; i < records.size(); i++)
    {
        try
        {
            categories.push_back(CategoryRecordCodec::parse(records[i]));
        }
        catch (const std::invalid_argument& error)
        {
            throw std::invalid_argument(filePath_ + ", record " + std::to_string(i + 1) +
                ": " + error.what());
        }
    }
    return categories;
}

void FileCategoryRepository::saveAll(const vector<ProductCategory>& categories)
{
    recoverInterruptedSave();

    const string temp = tempPath();
    const string backup = backupPath();

    bool written = false;
    {
        std::ofstream output(temp.c_str(), std::ios::trunc);
        if (!output)
            throw std::runtime_error("cannot create temporary file: " + temp);

        for (size_t i = 0; i < categories.size(); i++)
            output << CategoryRecordCodec::format(categories[i]) << "\n";

        output.close();
        written = !output.fail();
    }
    if (!written)
    {
        removeFile(temp);
        throw std::runtime_error("error while writing temporary file: " + temp +
            " (the category file was not changed)");
    }

    const bool hadOriginal = fileExists(filePath_);
    if (hadOriginal)
    {
        removeFile(backup);
        if (!renameFile(filePath_, backup))
        {
            removeFile(temp);
            throw std::runtime_error("cannot create backup " + backup +
                " (the category file was not changed)");
        }
    }

    if (!renameFile(temp, filePath_))
    {
        string message = "cannot put the new category file in place: " + filePath_;
        if (hadOriginal)
        {
            if (renameFile(backup, filePath_))
                message += " (the previous file was restored)";
            else
                message += " (the previous data is kept in " + backup + ")";
        }
        removeFile(temp);
        throw std::runtime_error(message);
    }

    if (hadOriginal)
        removeFile(backup);
}
