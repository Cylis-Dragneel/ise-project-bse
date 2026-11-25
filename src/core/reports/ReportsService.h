#ifndef REPORTS_SERVICE_H
#define REPORTS_SERVICE_H

#include <cstddef>
#include <string>
#include <vector>

#include "Bill.h"
#include "Order.h"

class ProductCatalog;
class ISeasonProvider;

namespace reports
{
// Cash-sales period selector. Daily/Weekly/Monthly/Yearly use `today` (the
// current calendar day/week/month/year); Custom uses an inclusive from/to
// range supplied by the caller.
enum class CashPeriod
{
    Daily,
    Weekly,
    Monthly,
    Yearly,
    Custom
};

struct OrderRow
{
    std::string orderId;
    std::string date;
    std::string customerName;
    std::string status;
    double total = 0;
};

struct CashRow
{
    // Daily/weekly/monthly/custom rows use YYYY-MM-DD; yearly rows use YYYY-MM.
    std::string period;
    double amount = 0;
    int orderCount = 0;
};

struct SalesRow
{
    std::string billId;
    std::string date;
    std::string customerName;
    std::string paymentStatus;
    double totalNoTax = 0;
    double totalTax = 0;
    double productDiscount = 0;
    double seasonalDiscount = 0;
    double specialDiscount = 0;
    double totalPayable = 0;
};

struct ItemRow
{
    std::string acronym;
    std::string name;
    double quantity = 0;       // sold quantity; 0 for the stock report
    double remainingStock = 0;
};

struct SalesTotals
{
    double totalPayable = 0;
    double productDiscount = 0;
    double seasonalDiscount = 0;
    double specialDiscount = 0;
};

// Qt-free manager reports (requirement 11). Reads Order.txt, OrderDetail.txt,
// Bill.txt and BillDetail.txt, and takes prices/stock from the catalog.
//
// Rules confirmed in the plan:
//   - incomplete = status other than paid/archived; total is the bill total if
//     the order already has a bill, otherwise a quote at current prices.
//   - completed = paid/archived; total comes from the bill.
//   - cash sales = paid orders with paymentMode "cash".
//   - top sold = completed bills only.
class ReportsService
{
public:
    ReportsService(const ProductCatalog &catalog,
                   std::string orderPath = "Order.txt",
                   std::string orderDetailPath = "OrderDetail.txt",
                   std::string billPath = "Bill.txt",
                   std::string billDetailPath = "BillDetail.txt",
                   const ISeasonProvider *seasons = nullptr);

    // Requirement 11.1. `totalOut` receives the sum of the row totals.
    std::vector<OrderRow> incompleteOrders(double *totalOut = nullptr) const;

    // Requirement 11.2.
    std::vector<OrderRow> completedOrders(double *totalOut = nullptr) const;

    // Requirement 11.3. Daily/Weekly/Monthly/Custom group by day; Yearly groups
    // by month. `today` defaults to the real local date.
    std::vector<CashRow> cashSales(CashPeriod period,
                                   const std::string &fromDate = "",
                                   const std::string &toDate = "",
                                   const std::string &today = "",
                                   double *totalOut = nullptr) const;

    // Requirement 11.4. `totalsOut` receives the column totals.
    std::vector<SalesRow> allSales(SalesTotals *totalsOut = nullptr) const;

    // Requirement 11.5. Top sold items by quantity, descending.
    std::vector<ItemRow> topSoldItems(std::size_t limit = 10) const;

    // Requirement 11.6. Top items by remaining stock, descending.
    std::vector<ItemRow> topStockItems(std::size_t limit = 10) const;

private:
    const ProductCatalog &catalog_;
    std::string orderPath_;
    std::string orderDetailPath_;
    std::string billPath_;
    std::string billDetailPath_;
    const ISeasonProvider *seasons_;

    std::vector<Bill> loadBills() const;
    double quoteOrder(const Order &order,
                      const std::vector<OrderDetail> &details) const;
};
} // namespace reports

#endif // REPORTS_SERVICE_H
