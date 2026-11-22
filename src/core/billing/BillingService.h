#ifndef BILLING_SERVICE_H
#define BILLING_SERVICE_H

#include <memory>
#include <string>
#include <vector>

#include "Bill.h"
#include "IBillDetailRepository.h"
#include "IBillRepository.h"
#include "Order.h"

class ProductCatalog;
class ISeasonProvider;

// Turns a saved order into a persisted bill:
//   order + its detail rows -> product data -> totals -> Bill.txt + BillDetail.txt
//
// Qt-free and independent of the GUI. The order is expected to be saved first
// (OrderManager::saveOrderTransaction) and to carry its final status; the bill
// mirrors that status. Stock is checked again here, so a cart that went stale
// cannot be billed. Stock is not deducted here: the order flow owns inventory.
class BillingService
{
public:
    BillingService(const ProductCatalog& catalog,
                   std::shared_ptr<IBillRepository> billRepository,
                   std::shared_ptr<IBillDetailRepository> detailRepository,
                   const ISeasonProvider* seasons = nullptr,
                   std::string detailFilePath = "OrderDetail.txt");

    // Builds and calculates the bill without writing it. Useful for the
    // running total on the order-entry screen; billId is left empty.
    // `billDate` defaults to today; `specialDiscountPercent` must be 0-8.
    Bill quote(const Order& order,
               const std::vector<OrderDetail>& details,
               const std::string& billDate = "",
               double specialDiscountPercent = 0) const;

    // Uses the details already in memory (the order-entry cart). Throws
    // std::invalid_argument for an invalid order / unknown product / amount
    // received below the total, std::out_of_range when stock is short,
    // std::runtime_error on file errors.
    Bill generateBill(const Order& order,
                      const std::vector<OrderDetail>& details,
                      double specialDiscountPercent = 0);

    // Reads the order's detail rows from detailFilePath and calls the overload
    // above. Throws std::invalid_argument when the order has no detail rows.
    Bill generateBill(const Order& order, double specialDiscountPercent = 0);

    // Replaces an existing bill and its detail rows (used after a paid order is
    // edited). The bill id stays the same.
    void replaceBill(const Bill& bill);

    // Loads Bill.txt and BillDetail.txt and joins them by bill id. Header
    // totals are authoritative; detail lines do not carry the tax.
    std::vector<Bill> loadBills();

private:
    const ProductCatalog& catalog_;
    std::shared_ptr<IBillRepository> billRepository_;
    std::shared_ptr<IBillDetailRepository> detailRepository_;
    const ISeasonProvider* seasons_;
    std::string detailFilePath_;
};

#endif // BILLING_SERVICE_H
