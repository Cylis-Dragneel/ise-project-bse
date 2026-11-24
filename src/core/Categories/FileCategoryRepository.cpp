#include "FileCategoryRepository.h"
#include "CategoryRecordCodec.h"
#include "RecordSplitter.h"
#include <stdexcept>

using std::string;
using std::vector;

FileCategoryRepository::FileCategoryRepository(const string& filePath)
    : store_(std::make_shared<SafeFileStore>(filePath))
{
}

FileCategoryRepository::FileCategoryRepository(std::shared_ptr<SafeFileStore> store)
    : store_(std::move(store))
{
    if (!store_)
        throw std::invalid_argument("FileCategoryRepository needs a file store");
}

vector<ProductCategory> FileCategoryRepository::loadAll()
{
    if (!store_->exists())
        return vector<ProductCategory>();

    vector<string> records;
    try
    {
        records = RecordSplitter::split(store_->read());
    }
    catch (const std::invalid_argument& e)
    {
        throw std::invalid_argument(store_->path() + ": " + e.what());
    }

    vector<ProductCategory> categories;
    for (size_t i = 0; i < records.size(); i++)
    {
        try
        {
            categories.push_back(CategoryRecordCodec::parse(records[i]));
        }
        catch (const std::invalid_argument& e)
        {
            throw std::invalid_argument(store_->path() + ", record " + std::to_string(i + 1) +
                ": " + e.what());
        }
    }
    return categories;
}

void FileCategoryRepository::saveAll(const vector<ProductCategory>& categories)
{
    string content;
    for (size_t i = 0; i < categories.size(); i++)
        content += CategoryRecordCodec::format(categories[i]) + "\n";
    store_->write(content);
}