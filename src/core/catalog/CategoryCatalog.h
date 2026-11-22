#pragma once
#ifndef CATEGORY_CATALOG_H
#define CATEGORY_CATALOG_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ICategoryRepository.h"
#include "ProductCategory.h"

// The business view of the product categories: look up by id, list, add and
// edit. Enforces uniqueness of ids and acronyms (case-insensitive). Knows
// nothing about files or GUI.
class CategoryCatalog
{
private:
    std::shared_ptr<ICategoryRepository> repository_;
    std::vector<ProductCategory> categories_;
    std::map<std::string, std::size_t> indexById_;

    void rebuildIndexes();
    void rejectDuplicateAcronym(const ProductCategory& category,
                                std::size_t ignoreIndex) const;

public:
    explicit CategoryCatalog(std::shared_ptr<ICategoryRepository> repository);

    // (Re)loads everything from the repository. Throws on unreadable data or
    // duplicate ids / acronyms. Pointers from earlier find...() calls become
    // invalid after load().
    void load();

    // nullptr when the category does not exist. The pointer stays valid until
    // the next load() or add/update.
    const ProductCategory* findById(const std::string& id) const;

    const std::vector<ProductCategory>& all() const { return categories_; }

    // Adds a category and persists it. Throws std::invalid_argument for a
    // duplicate id or acronym. On a save failure the in-memory list is
    // rolled back and the exception is rethrown.
    void add(const ProductCategory& category);

    // Replaces the category that has the same id (the id is the identity).
    // Throws std::invalid_argument when the id is unknown or the acronym
    // duplicates another category.
    void update(const ProductCategory& category);
};

#endif // CATEGORY_CATALOG_H
