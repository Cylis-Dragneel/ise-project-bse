#ifndef I_BILL_REPOSITORY_H
#define I_BILL_REPOSITORY_H

#include <vector>

#include "Bill.h"

// Storage of bill lines. BillingService only depends on this interface, so the
// tests can use an in-memory repository and the app uses a text file.
class IBillRepository
{
public:
    virtual ~IBillRepository() = default;

    // All lines currently stored, in file order. A missing file means "no
    // bills yet", not an error.
    virtual std::vector<BillLine> loadAll() = 0;

    // Appends the lines of one bill. All-or-nothing is not required: a bill is
    // a receipt, so a partial write is reported as an error.
    virtual void append(const std::vector<BillLine>& lines) = 0;
};

#endif // I_BILL_REPOSITORY_H
