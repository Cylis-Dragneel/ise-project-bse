#ifndef I_BILL_DETAIL_REPOSITORY_H
#define I_BILL_DETAIL_REPOSITORY_H

#include <vector>

#include "Bill.h"

// Storage of bill lines (BillDetail.txt). BillingService joins them to the
// headers by bill id.
class IBillDetailRepository
{
public:
    virtual ~IBillDetailRepository() = default;

    // All lines currently stored, in file order. A missing file means "no
    // details yet".
    virtual std::vector<BillLine> loadAll() = 0;

    // Replaces the whole file with `lines`.
    virtual void saveAll(const std::vector<BillLine>& lines) = 0;
};

#endif // I_BILL_DETAIL_REPOSITORY_H
