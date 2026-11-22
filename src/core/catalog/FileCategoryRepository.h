#ifndef FILE_CATEGORY_REPOSITORY_H
#define FILE_CATEGORY_REPOSITORY_H

#include <string>

#include "ICategoryRepository.h"

// Stores product categories in a text file (PCategory.txt). Only does file I/O;
// the format itself is handled by CategoryRecordCodec.
//
// Saving uses the same crash-safe rename sequence as FileProductRepository:
// write .tmp, move the old file to .bak, move .tmp into place, delete .bak.
class FileCategoryRepository : public ICategoryRepository
{
private:
    std::string filePath_;

    std::string tempPath() const { return filePath_ + ".tmp"; }
    std::string backupPath() const { return filePath_ + ".bak"; }

    void recoverInterruptedSave();

protected:
    // Virtual so tests can simulate a failing disk.
    virtual bool renameFile(const std::string& from, const std::string& to);
    virtual bool removeFile(const std::string& path);

public:
    explicit FileCategoryRepository(const std::string& filePath);

    // A missing file means "no categories yet" and returns an empty vector.
    // Throws std::runtime_error (cannot read) or std::invalid_argument (bad
    // record, message names the record number).
    std::vector<ProductCategory> loadAll() override;

    // Throws std::runtime_error on failure; in that case the previous file is
    // left (or put back) exactly as it was.
    void saveAll(const std::vector<ProductCategory>& categories) override;
};

#endif // FILE_CATEGORY_REPOSITORY_H
