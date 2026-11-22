#ifndef FILE_BILL_REPOSITORY_H
#define FILE_BILL_REPOSITORY_H

#include <string>

#include "IBillRepository.h"

// Stores bill headers in a text file (Bill.txt). Only does file I/O; the format
// itself is handled by BillRecordCodec.
//
// Saving uses the crash-safe rename sequence: write .tmp, move the old file to
// .bak, move .tmp into place, delete .bak.
class FileBillRepository : public IBillRepository
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
    explicit FileBillRepository(const std::string& filePath);

    // A missing file returns an empty vector.
    std::vector<Bill> loadAll() override;

    // Throws std::runtime_error on failure; the previous file is restored.
    void saveAll(const std::vector<Bill>& bills) override;
};

#endif // FILE_BILL_REPOSITORY_H
