#ifndef FILE_BILL_REPOSITORY_H
#define FILE_BILL_REPOSITORY_H

#include <string>
#include <vector>

#include "IBillRepository.h"

class FileBillRepository : public IBillRepository
{
private:
    std::string filePath_;

public:
    explicit FileBillRepository(const std::string& filePath = "Bill.txt");

    std::vector<Bill> loadAll() override;
    void saveAll(const std::vector<Bill>& bills) override;
    void append(const Bill& bill) override;
};

class FileBillDetailRepository : public IBillDetailRepository
{
private:
    std::string filePath_;

public:
    explicit FileBillDetailRepository(const std::string& filePath = "BillDetail.txt");

    std::vector<BillLine> loadAll() override;
    void saveAll(const std::vector<BillLine>& lines) override;
    void append(const std::vector<BillLine>& lines) override;
};

#endif // FILE_BILL_REPOSITORY_H