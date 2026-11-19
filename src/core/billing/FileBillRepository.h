#ifndef FILE_BILL_REPOSITORY_H
#define FILE_BILL_REPOSITORY_H

#include <string>

#include "IBillRepository.h"

// Stores bill lines in a text file (Bill.txt). Only does file I/O; the format
// itself is handled by BillRecordCodec.
class FileBillRepository : public IBillRepository
{
private:
    std::string filePath_;

public:
    explicit FileBillRepository(const std::string& filePath);

    // Throws std::runtime_error (cannot read) or std::invalid_argument (bad
    // record, message names the record number). A missing file returns an
    // empty vector.
    std::vector<BillLine> loadAll() override;

    // Throws std::runtime_error if the file cannot be opened or written.
    void append(const std::vector<BillLine>& lines) override;
};

#endif // FILE_BILL_REPOSITORY_H
