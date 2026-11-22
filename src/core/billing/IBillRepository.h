#ifndef I_BILL_REPOSITORY_H
#define I_BILL_REPOSITORY_H

#include <vector>

#include "Bill.h"

// Storage of bill headers (Bill.txt). BillingService only depends on this
// interface, so tests can use an in-memory repository and the app a text file.
class IBillRepository
{
public:
    virtual ~IBillRepository() = default;

    // All headers currently stored, in file order. A missing file means "no
    // bills yet", not an error. The returned bills have empty `lines`.
    virtual std::vector<Bill> loadAll() = 0;

    // Replaces the whole file with `bills`. The implementation must not lose
    // the old file when the write fails.
    virtual void saveAll(const std::vector<Bill>& bills) = 0;
};

#endif // I_BILL_REPOSITORY_H
