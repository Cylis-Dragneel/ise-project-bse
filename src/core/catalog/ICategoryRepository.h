#pragma once
#ifndef I_CATEGORY_REPOSITORY_H
#define I_CATEGORY_REPOSITORY_H

#include <vector>

#include "ProductCategory.h"

// Abstraction of "where categories are stored". CategoryCatalog depends only on
// this interface, so tests can use an in-memory fake and the app uses a file.
class ICategoryRepository
{
public:
    virtual ~ICategoryRepository() = default;
    virtual std::vector<ProductCategory> loadAll() = 0;
    virtual void saveAll(const std::vector<ProductCategory>& categories) = 0;
};

#endif // I_CATEGORY_REPOSITORY_H
