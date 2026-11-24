#ifndef FILE_BILL_DETAIL_REPOSITORY_H
#define FILE_BILL_DETAIL_REPOSITORY_H

#include <string>
#include <vector>

#include "IBillDetailRepository.h"

#ifndef FILE_BILL_DETAIL_REPOSITORY_DEFINED
#define FILE_BILL_DETAIL_REPOSITORY_DEFINED

class FileBillDetailRepository : public IBillDetailRepository
{
private:
    std::string filePath_;

public:
    explicit FileBillDetailRepository(const std::string& filePath = "BillDetail.txt");

    std::vector<BillLine> loadAll() override;
    void saveAll(const std::vector<BillLine>& lines) override;
    void append(const std::vector<BillLine>& lines);
};

#endif // FILE_BILL_DETAIL_REPOSITORY_DEFINED

#endif // FILE_BILL_DETAIL_REPOSITORY_H