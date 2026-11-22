#include "CategoryCatalog.h"

#include <cctype>
#include <stdexcept>

using std::string;
using std::vector;

namespace
{
    string lowerCopy(const string& text)
    {
        string lower;
        lower.reserve(text.size());
        for (size_t i = 0; i < text.size(); i++)
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
        return lower;
    }
}

CategoryCatalog::CategoryCatalog(std::shared_ptr<ICategoryRepository> repository)
    : repository_(std::move(repository))
{
    if (!repository_)
        throw std::invalid_argument("CategoryCatalog needs a repository");
}

void CategoryCatalog::rebuildIndexes()
{
    indexById_.clear();
    for (size_t i = 0; i < categories_.size(); i++)
        indexById_[categories_[i].id()] = i;
}

void CategoryCatalog::rejectDuplicateAcronym(const ProductCategory& category,
                                             std::size_t ignoreIndex) const
{
    const string wanted = lowerCopy(category.acronym());
    for (size_t i = 0; i < categories_.size(); i++)
    {
        if (i == ignoreIndex) continue;
        if (lowerCopy(categories_[i].acronym()) == wanted)
            throw std::invalid_argument("duplicate category acronym: " +
                category.acronym());
    }
}

void CategoryCatalog::load()
{
    vector<ProductCategory> loaded = repository_->loadAll();

    std::map<string, std::size_t> byId;
    std::map<string, std::size_t> byAcronym;
    for (size_t i = 0; i < loaded.size(); i++)
    {
        if (!byId.insert(std::make_pair(loaded[i].id(), i)).second)
            throw std::invalid_argument("duplicate category id: " + loaded[i].id());

        if (!byAcronym.insert(std::make_pair(lowerCopy(loaded[i].acronym()), i)).second)
            throw std::invalid_argument("duplicate category acronym: " +
                loaded[i].acronym());
    }

    // Only replace the current data once everything is known to be valid.
    categories_.swap(loaded);
    indexById_.swap(byId);
}

const ProductCategory* CategoryCatalog::findById(const string& id) const
{
    auto found = indexById_.find(id);
    return found == indexById_.end() ? nullptr : &categories_[found->second];
}

void CategoryCatalog::add(const ProductCategory& category)
{
    if (indexById_.find(category.id()) != indexById_.end())
        throw std::invalid_argument("duplicate category id: " + category.id());
    rejectDuplicateAcronym(category, categories_.size());

    const vector<ProductCategory> backup = categories_;
    categories_.push_back(category);
    rebuildIndexes();

    try
    {
        repository_->saveAll(categories_);
    }
    catch (...)
    {
        categories_ = backup;
        rebuildIndexes();
        throw;
    }
}

void CategoryCatalog::update(const ProductCategory& category)
{
    auto found = indexById_.find(category.id());
    if (found == indexById_.end())
        throw std::invalid_argument("unknown category id: " + category.id());

    rejectDuplicateAcronym(category, found->second);

    const vector<ProductCategory> backup = categories_;
    categories_[found->second] = category;
    rebuildIndexes();

    try
    {
        repository_->saveAll(categories_);
    }
    catch (...)
    {
        categories_ = backup;
        rebuildIndexes();
        throw;
    }
}
