#ifndef FILEPRODUCTREPOSITORY_H
#define FILEPRODUCTREPOSITORY_H

#include "IProductRepository.h"
#include "Product.h"
#include <string>
#include <vector>

class FileProductRepository : public IProductRepository {
public:
    explicit FileProductRepository(const std::string& path);

    std::vector<Product> loadAll() override;
    void saveAll(const std::vector<Product>& products) override;

    const std::string& path() const;

protected:
    virtual bool renameFile(const std::string& from, const std::string& to);
    virtual bool removeFile(const std::string& path);

private:
    void recoverIfNeeded();
    bool fileExists(const std::string& path);

    std::string path_;
    std::string tempPath_;
    std::string backupPath_;
};

#endif