#ifndef BILLING_SERVICE_H
#define BILLING_SERVICE_H

#include <memory>
#include <string>
#include <vector>

#include "Bill.h"
#include "IBillRepository.h"
#include "Order.h"
#include "catalog/Product.h"
#include "seasons/ISeasonProvider.h"

class ProductCatalog;
class SeasonService;

class BillingService
{
public:
    BillingService(const ProductCatalog& catalog,
                   std::shared_ptr<IBillRepository> billRepository,
                   std::shared_ptr<IBillDetailRepository> detailRepository,
                   std::shared_ptr<ISeasonProvider> seasonProvider = nullptr,
                   std::string orderDetailFilePath = "OrderDetail.txt");

    BillingService(const ProductCatalog& catalog,
                   std::shared_ptr<IBillRepository> billRepository,
                   std::shared_ptr<IBillDetailRepository> detailRepository,
                   SeasonService* seasonService,
                   std::string orderDetailFilePath = "OrderDetail.txt");

    BillingService(const ProductCatalog& catalog,
                   std::shared_ptr<IBillRepository> billRepository,
                   SeasonService* seasonService,
                   std::string orderDetailFilePath = "OrderDetail.txt",
                   std::shared_ptr<IBillDetailRepository> detailRepository = nullptr);

    // `checkStock` re-checks the catalog before pricing. Keep it true when the
    // caller has NOT reserved stock yet (a fresh cart). Pass false after
    // OrderService has reserved the order, otherwise the reserved quantity is
    // counted twice and a large sale is rejected.
    Bill quote(const Order& order,
               const std::vector<OrderDetail>& details,
               double specialDiscountPercent = 0.0,
               double amountReceived = 0.0,
               const std::string& billDate = "",
               const std::string& billTime = "",
               bool checkStock = true) const;

    Bill generateBill(const Order& order,
                      const std::vector<OrderDetail>& details,
                      double specialDiscountPercent = 0.0,
                      double amountReceived = 0.0,
                      const std::string& billDate = "",
                      const std::string& billTime = "",
                      bool checkStock = true);

    Bill generateBill(const Order& order,
                      double specialDiscountPercent = 0.0,
                      double amountReceived = 0.0,
                      const std::string& billDate = "",
                      const std::string& billTime = "",
                      bool checkStock = true);

    Bill updateExistingBill(const std::string& existingBillId,
                            const Order& updatedOrder,
                            const std::vector<OrderDetail>& updatedDetails,
                            double specialDiscountPercent = 0.0,
                            double amountReceived = 0.0,
                            const std::string& newBillDate = "",
                            const std::string& newBillTime = "");

    std::vector<Bill> loadBills() const;

    bool findBillById(const std::string& billId, Bill& outBill) const;
    bool findBillByOrderId(const std::string& orderId, Bill& outBill) const;

private:
    const ProductCatalog& catalog_;
    std::shared_ptr<IBillRepository> billRepository_;
    std::shared_ptr<IBillDetailRepository> detailRepository_;
    std::shared_ptr<ISeasonProvider> seasonProvider_;
    SeasonService* rawSeasonService_ = nullptr;
    std::string orderDetailFilePath_;

    double resolveSeasonPercent(const std::string& effectiveDate) const;

    std::vector<Product> resolveProducts(const Order& order,
                                         const std::vector<OrderDetail>& details,
                                         bool checkStock) const;
};

#endif // BILLING_SERVICE_H