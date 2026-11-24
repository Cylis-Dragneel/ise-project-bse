#pragma once
#ifndef I_CATEGORY_REPOSITORY_H
#define I_CATEGORY_REPOSITORY_H

#include <vector>
#include "ProductCategory.h"

class ICategoryRepository
{
public:
    virtual ~ICategoryRepository() = default;
    virtual std::vector<ProductCategory> loadAll() = 0;
    virtual void saveAll(const std::vector<ProductCategory>& categories) = 0;
};

#endif