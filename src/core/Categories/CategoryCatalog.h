#pragma once
#ifndef CATEGORY_CATALOG_H
#define CATEGORY_CATALOG_H

#include <memory>
#include <string>
#include <vector>
#include "ICategoryRepository.h"
#include "ProductCategory.h"

class CategoryCatalog
{
private:
    std::shared_ptr<ICategoryRepository> repository_;
    std::vector<ProductCategory> categories_;

    static void requireUnique(const std::vector<ProductCategory>& list);
    void commit(std::vector<ProductCategory>& candidate);

public:
    explicit CategoryCatalog(std::shared_ptr<ICategoryRepository> repository);

    void load();

    const std::vector<ProductCategory>& all() const { return categories_; }
    const ProductCategory* findById(const std::string& id) const;
    const ProductCategory* findByAcronym(const std::string& acronym) const;

    void add(const ProductCategory& category);
    void update(const std::string& id, const std::string& newName, const std::string& newAcronym);
};

#endif