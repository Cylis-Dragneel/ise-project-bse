#ifndef I_BILL_REPOSITORY_H
#define I_BILL_REPOSITORY_H

#include <vector>

#include "Bill.h"

// ============================================================================
// INCREMENT 2 REPOSITORY INTERFACE (Preserved)
// ============================================================================
class IBillRepository
{
public:
    virtual ~IBillRepository() = default;
    virtual std::vector<BillLine> loadAll() = 0;
    virtual void append(const std::vector<BillLine>& lines) = 0;
};

// ============================================================================
// INCREMENT 3 REPOSITORY INTERFACES (14-field Bill.txt & 5-field BillDetail.txt)
// ============================================================================
namespace inc3 {

class IBillHeaderRepository
{
public:
    virtual ~IBillHeaderRepository() = default;
    virtual std::vector<BillHeader> loadAll() = 0;
    virtual void saveAll(const std::vector<BillHeader>& bills) = 0;
    virtual void append(const BillHeader& bill) = 0;
};

class IBillDetailRepository
{
public:
    virtual ~IBillDetailRepository() = default;
    virtual std::vector<BillDetailLine> loadAll() = 0;
    virtual void saveAll(const std::vector<BillDetailLine>& lines) = 0;
    virtual void append(const std::vector<BillDetailLine>& lines) = 0;
};

} // namespace inc3

#endif // I_BILL_REPOSITORY_H
