#ifndef I_BILL_REPOSITORY_H
#define I_BILL_REPOSITORY_H

#include <vector>

#include "Bill.h"

class IBillRepository
{
public:
    virtual ~IBillRepository() = default;
    virtual std::vector<Bill> loadAll() = 0;
    virtual void saveAll(const std::vector<Bill>& bills) = 0;
    virtual void append(const Bill& bill) = 0;
};

class IBillDetailRepository
{
public:
    virtual ~IBillDetailRepository() = default;
    virtual std::vector<BillLine> loadAll() = 0;
    virtual void saveAll(const std::vector<BillLine>& lines) = 0;
    virtual void append(const std::vector<BillLine>& lines) = 0;
};

#endif // I_BILL_REPOSITORY_H