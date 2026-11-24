#pragma once
#ifndef PRODUCT_CATEGORY_H
#define PRODUCT_CATEGORY_H

#include <cstddef>
#include <string>

class ProductCategory
{
private:
    std::string id_;
    std::string name_;
    std::string acronym_;

public:
    static std::size_t idLength() { return 2; }
    static std::size_t maxAcronymLength() { return 8; }

    ProductCategory(const std::string& id,
        const std::string& name,
        const std::string& acronym);

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& acronym() const { return acronym_; }

    ProductCategory withDetails(const std::string& newName,
        const std::string& newAcronym) const;
};

#endif