#include "CategoryCatalog.h"
#include "FieldRules.h"
#include <stdexcept>

using std::string;
using std::vector;

CategoryCatalog::CategoryCatalog(std::shared_ptr<ICategoryRepository> repository)
    : repository_(std::move(repository))
{
    if (!repository_)
        throw std::invalid_argument("CategoryCatalog needs a repository");
}

void CategoryCatalog::requireUnique(const vector<ProductCategory>& list)
{
    for (size_t i = 0; i < list.size(); i++)
    {
        for (size_t j = i + 1; j < list.size(); j++)
        {
            if (list[i].id() == list[j].id())
                throw std::invalid_argument("duplicate category id: " + list[i].id());
            if (FieldRules::equalsIgnoreCase(list[i].acronym(), list[j].acronym()))
                throw std::invalid_argument("duplicate category acronym: " + list[i].acronym());
        }
    }
}

void CategoryCatalog::commit(vector<ProductCategory>& candidate)
{
    requireUnique(candidate);
    repository_->saveAll(candidate);
    categories_.swap(candidate);
}

void CategoryCatalog::load()
{
    vector<ProductCategory> loaded = repository_->loadAll();
    requireUnique(loaded);
    categories_.swap(loaded);
}

const ProductCategory* CategoryCatalog::findById(const string& id) const
{
    for (size_t i = 0; i < categories_.size(); i++)
        if (categories_[i].id() == id) return &categories_[i];
    return nullptr;
}

const ProductCategory* CategoryCatalog::findByAcronym(const string& acronym) const
{
    for (size_t i = 0; i < categories_.size(); i++)
        if (FieldRules::equalsIgnoreCase(categories_[i].acronym(), acronym)) return &categories_[i];
    return nullptr;
}

void CategoryCatalog::add(const ProductCategory& category)
{
    vector<ProductCategory> candidate = categories_;
    candidate.push_back(category);
    commit(candidate);
}

void CategoryCatalog::update(const string& id, const string& newName, const string& newAcronym)
{
    vector<ProductCategory> candidate = categories_;
    for (size_t i = 0; i < candidate.size(); i++)
    {
        if (candidate[i].id() == id)
        {
            candidate[i] = candidate[i].withDetails(newName, newAcronym);
            commit(candidate);
            return;
        }
    }
    throw std::out_of_range("unknown category id: " + id);
}