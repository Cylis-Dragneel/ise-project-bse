#ifndef FILE_PRODUCT_REPOSITORY_H
#define FILE_PRODUCT_REPOSITORY_H

#include <string>
#include "IProductRepository.h"

// Stores products in a text file (Product.txt). Only does file I/O;
// the format itself is handled by ProductRecordCodec.
//
// Saving never destroys the existing file before the new one is in place:
//   1. write the new content to  <file>.tmp
//   2. rename the current file   <file>  ->  <file>.bak
//   3. rename                    <file>.tmp -> <file>
//   4. success: delete <file>.bak      failure: rename <file>.bak back to <file>
class FileProductRepository : public IProductRepository
{
private:
    std::string filePath_;

    std::string tempPath() const { return filePath_ + ".tmp"; }
    std::string backupPath() const { return filePath_ + ".bak"; }

    // If the program died in the middle of a save (file missing but the
    // backup exists), put the backup back.
    void recoverInterruptedSave();

protected:
    // The only two file-system operations used for saving. Virtual so tests
    // can simulate a failing disk.
    virtual bool renameFile(const std::string& from, const std::string& to);
    virtual bool removeFile(const std::string& path);

public:
    explicit FileProductRepository(const std::string& filePath);

    // Throws std::runtime_error (cannot open file) or
    // std::invalid_argument (bad record, message names the record number).
    std::vector<Product> loadAll() override;

    // Throws std::runtime_error on failure; in that case the previous file
    // is left (or put back) exactly as it was.
    void saveAll(const std::vector<Product>& products) override;
};

#endif