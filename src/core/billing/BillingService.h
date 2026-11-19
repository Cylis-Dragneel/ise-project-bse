#ifndef BILLING_SERVICE_H
#define BILLING_SERVICE_H

#include <memory>
#include <string>
#include <vector>

#include "Bill.h"
#include "IBillRepository.h"
#include "Order.h"

class ProductCatalog;

// Turns a saved order into a persisted bill:
//   order + its detail rows -> product data -> totals -> Bill.txt
//
// Qt-free and independent of the GUI. The order is expected to be saved first
// (OrderManager::saveOrderTransaction) and to carry its final status; the bill
// mirrors that status. Stock is checked again here, so a cart that went stale
// cannot be billed. Stock is not deducted here: the order flow owns inventory
// and can call ProductCatalog::deductStock() once the sale is committed.
class BillingService
{
public:
    BillingService(const ProductCatalog& catalog,
                   std::shared_ptr<IBillRepository> billRepository,
                   std::string detailFilePath = "OrderDetail.txt");

    // Builds and calculates the bill without writing it. Useful for the
    // running total on the order-entry screen; billId is left empty.
    Bill quote(const Order& order, const std::vector<OrderDetail>& details) const;

    // Uses the details already in memory (the order-entry cart). Throws
    // std::invalid_argument for an invalid order / unknown product,
    // std::out_of_range when stock is short, std::runtime_error on file errors.
    Bill generateBill(const Order& order, const std::vector<OrderDetail>& details);

    // Reads the order's detail rows from detailFilePath and calls the overload
    // above. Throws std::invalid_argument when the order has no detail rows.
    Bill generateBill(const Order& order);

    // Re-groups Bill.txt into bills and recalculates every total.
    std::vector<Bill> loadBills();

private:
    const ProductCatalog& catalog_;
    std::shared_ptr<IBillRepository> billRepository_;
    std::string detailFilePath_;
};

#endif // BILLING_SERVICE_H
