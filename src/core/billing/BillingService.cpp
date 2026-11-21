#include "BillingService.h"

#include "BillCalculator.h"
#include "OrderManager.h"
#include "catalog/Product.h"
#include "catalog/ProductCatalog.h"

#include <map>
#include <stdexcept>

using std::string;
using std::vector;

BillingService::BillingService(const ProductCatalog& catalog,
                               std::shared_ptr<IBillRepository> billRepository,
                               string detailFilePath)
    : catalog_(catalog),
      billRepository_(std::move(billRepository)),
      detailFilePath_(std::move(detailFilePath))
{
    if (!billRepository_)
    {
        throw std::invalid_argument("BillingService needs a bill repository");
    }
}

Bill BillingService::quote(const Order& order,
                           const vector<OrderDetail>& details) const
{
    if (!OrderManager::validateOrder(order, details))
    {
        throw std::invalid_argument("cannot bill an invalid order: " + order.orderId);
    }

    // The same product may appear on several rows. Check the total, not each
    // row on its own, so 3 + 3 cannot slip past a stock of 5.
    std::map<string, double> requestedByBarcode;
    for (const OrderDetail& detail : details)
    {
        requestedByBarcode[detail.productBarcode] += detail.quantity;
    }

    for (const auto& request : requestedByBarcode)
    {
        const Product* product = catalog_.findByBarcode(request.first);
        if (product == nullptr)
        {
            throw std::invalid_argument("order " + order.orderId +
                                        " references unknown barcode " + request.first);
        }

        if (!product->hasStockFor(request.second))
        {
            throw std::out_of_range("not enough stock of '" + product->acronym() +
                                    "' for barcode " + request.first);
        }
    }

    Bill bill;
    bill.orderId = order.orderId;
    bill.paymentStatus = order.status;

    for (const OrderDetail& detail : details)
    {
        const Product* product = catalog_.findByBarcode(detail.productBarcode);
        bill.lines.push_back(
            BillCalculator::makeLine("", order.orderId, detail, *product, order.status));
    }

    BillCalculator::calculate(bill);
    return bill;
}

Bill BillingService::generateBill(const Order& order,
                                  const vector<OrderDetail>& details)
{
    Bill bill = quote(order, details);

    // Only a persisted bill gets an id.
    bill.billId = BillCalculator::nextBillId(billRepository_->loadAll());
    for (BillLine& line : bill.lines)
    {
        line.billId = bill.billId;
    }

    // Only persist once every line is known to be valid and calculated.
    billRepository_->append(bill.lines);
    return bill;
}

Bill BillingService::generateBill(const Order& order)
{
    const vector<OrderDetail> details =
        OrderManager::loadOrderDetails(order.orderId, detailFilePath_);

    if (details.empty())
    {
        throw std::invalid_argument("order " + order.orderId +
                                    " has no detail rows in " + detailFilePath_);
    }

    return generateBill(order, details);
}

vector<Bill> BillingService::loadBills()
{
    return BillCalculator::group(billRepository_->loadAll());
}
