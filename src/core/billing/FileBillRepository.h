#ifndef FILE_BILL_REPOSITORY_H
#define FILE_BILL_REPOSITORY_H

#include <string>
#include <vector>

#include "IBillRepository.h"

// ============================================================================
// INCREMENT 2 FILE REPOSITORY (Preserved)
// ============================================================================
class FileBillRepository : public IBillRepository
{
private:
    std::string filePath_;

public:
    explicit FileBillRepository(const std::string& filePath);
    std::vector<BillLine> loadAll() override;
    void append(const std::vector<BillLine>& lines) override;
};

// ============================================================================
// INCREMENT 3 FILE REPOSITORIES (Crash-safe atomic writes + unique billId)
// ============================================================================
namespace inc3 {

class FileBillHeaderRepository : public IBillHeaderRepository
{
private:
    std::string filePath_;

public:
    explicit FileBillHeaderRepository(const std::string& filePath = "Bill.txt");

    // Rejects duplicate billIds on load with std::invalid_argument.
    std::vector<BillHeader> loadAll() override;

    // Crash-safe temp-file + rename write. Rejects duplicate billIds.
    void saveAll(const std::vector<BillHeader>& bills) override;

    // Appends one bill header. Rejects duplicate billId with std::invalid_argument.
    void append(const BillHeader& bill) override;
};

class FileBillDetailRepository : public IBillDetailRepository
{
private:
    std::string filePath_;

public:
    explicit FileBillDetailRepository(const std::string& filePath = "BillDetail.txt");

    std::vector<BillDetailLine> loadAll() override;
    void saveAll(const std::vector<BillDetailLine>& lines) override;
    void append(const std::vector<BillDetailLine>& lines) override;
};

} // namespace inc3

#endif // FILE_BILL_REPOSITORY_H
