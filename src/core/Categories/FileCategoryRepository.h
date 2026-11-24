#pragma once
#ifndef FILE_CATEGORY_REPOSITORY_H
#define FILE_CATEGORY_REPOSITORY_H

#include <memory>
#include <string>
#include "ICategoryRepository.h"
#include "SafeFileStore.h"

class FileCategoryRepository : public ICategoryRepository
{
private:
    std::shared_ptr<SafeFileStore> store_;

public:
    explicit FileCategoryRepository(const std::string& filePath);
    explicit FileCategoryRepository(std::shared_ptr<SafeFileStore> store);

    std::vector<ProductCategory> loadAll() override;
    void saveAll(const std::vector<ProductCategory>& categories) override;
};

#endif