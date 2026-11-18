#pragma once
#ifndef I_PRODUCT_REPOSITORY_H
#define I_PRODUCT_REPOSITORY_H

#include <vector>
#include "Product.h"

// Abstraction of "where products are stored".
// Today: a text file. Tomorrow: a database, a network service, a test fake...
// ProductCatalog depends only on this interface (Open/Closed + Dependency Inversion).
class IProductRepository
{
public:
    virtual ~IProductRepository() = default;
    virtual std::vector<Product> loadAll() = 0;
    virtual void saveAll(const std::vector<Product>& products) = 0;
};

#endif