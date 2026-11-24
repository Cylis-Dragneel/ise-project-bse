#ifndef BILLING_SERVICE_H
#define BILLING_SERVICE_H

#include <memory>
#include <string>
#include <vector>

#include "Bill.h"
#include "IBillRepository.h"
#include "Order.h"

class ProductCatalog;

// ============================================================================
// INCREMENT 2 BILLING SERVICE (Preserved)
// ============================================================================
class BillingService
{
public:
    BillingService(const ProductCatalog& catalog,
                   std::shared_ptr<IBillRepository> billRepository,
                   std::string detailFilePath = "OrderDetail.txt");

    Bill quote(const Order& order, const std::vector<OrderDetail>& details) const;
    Bill generateBill(const Order& order, const std::vector<OrderDetail>& details);
    Bill generateBill(const Order& order);
    std::vector<Bill> loadBills();

private:
    const ProductCatalog& catalog_;
    std::shared_ptr<IBillRepository> billRepository_;
    std::string detailFilePath_;
};

// ============================================================================
// INCREMENT 3 BILLING SERVICE (Seasonal + Special Discounts + BillDetail.txt)
// ============================================================================
namespace inc3 {

class BillingService
{
public:
    BillingService(const ProductCatalog& catalog,
                   std::shared_ptr<IBillHeaderRepository> headerRepository,
                   std::shared_ptr<IBillDetailRepository> detailRepository,
                   std::shared_ptr<ISeasonProvider> seasonProvider = nullptr,
                   std::string orderDetailFilePath = "OrderDetail.txt");

    // Computes a bill preview without writing to disk (billId stays empty).
    BillHeader quote(const Order& order,
                     const std::vector<OrderDetail>& details,
                     double specialDiscountPercent = 0.0,
                     double amountReceived = 0.0,
                     const std::string& billDate = "",
                     const std::string& billTime = "") const;

    // Persists a new bill: writes BillDetail.txt first, then Bill.txt (rolls back
    // BillDetail.txt if Bill.txt fails).
    BillHeader generateBill(const Order& order,
                            const std::vector<OrderDetail>& details,
                            double specialDiscountPercent = 0.0,
                            double amountReceived = 0.0,
                            const std::string& billDate = "",
                            const std::string& billTime = "");

    // Overload loading order details from orderDetailFilePath_.
    BillHeader generateBill(const Order& order,
                            double specialDiscountPercent = 0.0,
                            double amountReceived = 0.0,
                            const std::string& billDate = "",
                            const std::string& billTime = "");

    // Updates an existing bill in place when a paid order is edited:
    // keeps the same billId, recomputes totals, replaces its BillDetail.txt rows,
    // and refreshes billDate / billTime.
    BillHeader updateExistingBill(const std::string& existingBillId,
                                  const Order& updatedOrder,
                                  const std::vector<OrderDetail>& updatedDetails,
                                  double specialDiscountPercent = 0.0,
                                  double amountReceived = 0.0,
                                  const std::string& newBillDate = "",
                                  const std::string& newBillTime = "");

    // Loads all bills from Bill.txt joined with their BillDetailLine items by billId.
    std::vector<BillHeader> loadBills() const;

    // Lookup helpers for UI / OrderPolicy 7-day rule / PDF reprint.
    bool findBillById(const std::string& billId, BillHeader& outBill) const;
    bool findBillByOrderId(const std::string& orderId, BillHeader& outBill) const;

private:
    const ProductCatalog& catalog_;
    std::shared_ptr<IBillHeaderRepository> headerRepository_;
    std::shared_ptr<IBillDetailRepository> detailRepository_;
    std::shared_ptr<ISeasonProvider> seasonProvider_;
    std::string orderDetailFilePath_;

    std::vector<Product> resolveProducts(const Order& order,
                                         const std::vector<OrderDetail>& details,
                                         bool checkStock) const;
};

} // namespace inc3

#endif // BILLING_SERVICE_H
