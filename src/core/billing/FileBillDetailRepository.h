#ifndef FILE_BILL_DETAIL_REPOSITORY_H
#define FILE_BILL_DETAIL_REPOSITORY_H

#include <string>

#include "IBillDetailRepository.h"

// Stores bill lines in a text file (BillDetail.txt). Only does file I/O; the
// format itself is handled by BillDetailRecordCodec.
class FileBillDetailRepository : public IBillDetailRepository
{
private:
    std::string filePath_;

    std::string tempPath() const { return filePath_ + ".tmp"; }
    std::string backupPath() const { return filePath_ + ".bak"; }

    void recoverInterruptedSave();

protected:
    virtual bool renameFile(const std::string& from, const std::string& to);
    virtual bool removeFile(const std::string& path);

public:
    explicit FileBillDetailRepository(const std::string& filePath);

    std::vector<BillLine> loadAll() override;
    void saveAll(const std::vector<BillLine>& lines) override;
};

#endif // FILE_BILL_DETAIL_REPOSITORY_H
