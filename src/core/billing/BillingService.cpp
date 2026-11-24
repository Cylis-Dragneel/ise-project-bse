#include "BillingService.h"

#include "BillCalculator.h"
#include "BillRecordCodec.h"
#include "OrderManager.h"
#include "catalog/Product.h"
#include "catalog/ProductCatalog.h"

#include <map>
#include <stdexcept>
#include <unordered_map>

using std::string;
using std::vector;

// ============================================================================
// INCREMENT 2 IMPLEMENTATION
// ============================================================================

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
    bill.billId = BillCalculator::nextBillId(billRepository_->loadAll());
    for (BillLine& line : bill.lines)
    {
        line.billId = bill.billId;
    }
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

// ============================================================================
// INCREMENT 3 IMPLEMENTATION
// ============================================================================

namespace inc3 {

BillingService::BillingService(const ProductCatalog& catalog,
                               std::shared_ptr<IBillHeaderRepository> headerRepository,
                               std::shared_ptr<IBillDetailRepository> detailRepository,
                               std::shared_ptr<ISeasonProvider> seasonProvider,
                               string orderDetailFilePath)
    : catalog_(catalog),
      headerRepository_(std::move(headerRepository)),
      detailRepository_(std::move(detailRepository)),
      seasonProvider_(std::move(seasonProvider)),
      orderDetailFilePath_(std::move(orderDetailFilePath))
{
    if (!headerRepository_ || !detailRepository_)
    {
        throw std::invalid_argument("inc3::BillingService requires header and detail repositories");
    }
}

vector<Product> BillingService::resolveProducts(const Order& order,
                                                const vector<OrderDetail>& details,
                                                bool checkStock) const
{
    if (!OrderManager::validateOrder(order, details))
    {
        throw std::invalid_argument("cannot bill an invalid order: " + order.orderId);
    }

    if (checkStock)
    {
        std::map<string, double> requestedByBarcode;
        for (const OrderDetail& detail : details)
        {
            requestedByBarcode[detail.productBarcode] += detail.quantity;
        }
        for (const auto& req : requestedByBarcode)
        {
            const Product* product = catalog_.findByBarcode(req.first);
            if (product == nullptr)
            {
                throw std::invalid_argument("order " + order.orderId +
                                            " references unknown barcode " + req.first);
            }
            if (!product->hasStockFor(req.second))
            {
                throw std::out_of_range("not enough stock of '" + product->acronym() +
                                        "' for barcode " + req.first);
            }
        }
    }

    vector<Product> resolved;
    resolved.reserve(details.size());
    for (const OrderDetail& detail : details)
    {
        const Product* product = catalog_.findByBarcode(detail.productBarcode);
        if (product == nullptr)
        {
            throw std::invalid_argument("order " + order.orderId +
                                        " references unknown barcode " + detail.productBarcode);
        }
        resolved.push_back(*product);
    }
    return resolved;
}

BillHeader BillingService::quote(const Order& order,
                                 const vector<OrderDetail>& details,
                                 double specialDiscountPercent,
                                 double amountReceived,
                                 const string& billDate,
                                 const string& billTime) const
{
    const vector<Product> products = resolveProducts(order, details, true);

    const string effectiveDate = billDate.empty()
                                     ? (order.date.empty() ? OrderManager::getCurrentDate() : order.date)
                                     : billDate;
    const string effectiveTime = billTime.empty()
                                     ? (order.time.empty() ? OrderManager::getCurrentTime() : order.time)
                                     : billTime;

    const double seasonPct = seasonProvider_ ? seasonProvider_->activePercent(effectiveDate) : 0.0;

    BillHeader bill;
    bill.orderId = order.orderId;
    bill.customerName = BillHeaderCodec::sanitizeCustomerName(order.customerName);
    bill.paymentStatus = order.status;
    bill.billDate = effectiveDate;
    bill.billTime = effectiveTime;

    const double effectiveReceived = (amountReceived == 0.0 && order.status != "paid")
                                         ? 0.0
                                         : amountReceived;

    BillingCalculator::calculate(bill, details, products, seasonPct,
                                 specialDiscountPercent, effectiveReceived);
    return bill;
}

BillHeader BillingService::generateBill(const Order& order,
                                        const vector<OrderDetail>& details,
                                        double specialDiscountPercent,
                                        double amountReceived,
                                        const string& billDate,
                                        const string& billTime)
{
    BillHeader bill = quote(order, details, specialDiscountPercent, amountReceived, billDate, billTime);
    bill.billId = BillingCalculator::nextBillId(headerRepository_->loadAll());
    for (BillDetailLine& line : bill.lines)
    {
        line.billId = bill.billId;
    }

    // Snapshot existing details so we can roll back if header persistence fails.
    const vector<BillDetailLine> previousDetails = detailRepository_->loadAll();

    detailRepository_->append(bill.lines);
    try
    {
        headerRepository_->append(bill);
    }
    catch (...)
    {
        detailRepository_->saveAll(previousDetails);
        throw;
    }

    return bill;
}

BillHeader BillingService::generateBill(const Order& order,
                                        double specialDiscountPercent,
                                        double amountReceived,
                                        const string& billDate,
                                        const string& billTime)
{
    const vector<OrderDetail> details =
        OrderManager::loadOrderDetails(order.orderId, orderDetailFilePath_);

    if (details.empty())
    {
        throw std::invalid_argument("order " + order.orderId +
                                    " has no detail rows in " + orderDetailFilePath_);
    }
    return generateBill(order, details, specialDiscountPercent, amountReceived, billDate, billTime);
}

BillHeader BillingService::updateExistingBill(const string& existingBillId,
                                              const Order& updatedOrder,
                                              const vector<OrderDetail>& updatedDetails,
                                              double specialDiscountPercent,
                                              double amountReceived,
                                              const string& newBillDate,
                                              const string& newBillTime)
{
    vector<BillHeader> allBills = headerRepository_->loadAll();
    std::size_t targetIdx = allBills.size();
    for (std::size_t i = 0; i < allBills.size(); ++i)
    {
        if (allBills[i].billId == existingBillId)
        {
            targetIdx = i;
            break;
        }
    }
    if (targetIdx == allBills.size())
    {
        throw std::invalid_argument("existing bill id not found: " + existingBillId);
    }

    // Stock adjustment is owned by Person B on order edit; resolve products here.
    const vector<Product> products = resolveProducts(updatedOrder, updatedDetails, false);

    const string effectiveDate = newBillDate.empty() ? OrderManager::getCurrentDate() : newBillDate;
    const string effectiveTime = newBillTime.empty() ? OrderManager::getCurrentTime() : newBillTime;
    const double seasonPct = seasonProvider_ ? seasonProvider_->activePercent(effectiveDate) : 0.0;

    BillHeader updatedBill;
    updatedBill.billId = existingBillId;
    updatedBill.orderId = updatedOrder.orderId;
    updatedBill.customerName = BillHeaderCodec::sanitizeCustomerName(updatedOrder.customerName);
    updatedBill.paymentStatus = updatedOrder.status;
    updatedBill.billDate = effectiveDate;
    updatedBill.billTime = effectiveTime;

    BillingCalculator::calculate(updatedBill, updatedDetails, products, seasonPct,
                                 specialDiscountPercent, amountReceived);

    const vector<BillDetailLine> previousDetails = detailRepository_->loadAll();
    vector<BillDetailLine> replacedDetails;
    replacedDetails.reserve(previousDetails.size() + updatedBill.lines.size());

    for (const BillDetailLine& line : previousDetails)
    {
        if (line.billId != existingBillId)
        {
            replacedDetails.push_back(line);
        }
    }
    replacedDetails.insert(replacedDetails.end(), updatedBill.lines.begin(), updatedBill.lines.end());

    // Write BillDetail.txt first, then Bill.txt with rollback on failure.
    detailRepository_->saveAll(replacedDetails);
    try
    {
        allBills[targetIdx] = updatedBill;
        headerRepository_->saveAll(allBills);
    }
    catch (...)
    {
        detailRepository_->saveAll(previousDetails);
        throw;
    }

    return updatedBill;
}

vector<BillHeader> BillingService::loadBills() const
{
    vector<BillHeader> bills = headerRepository_->loadAll();
    const vector<BillDetailLine> allLines = detailRepository_->loadAll();

    std::unordered_map<string, std::size_t> indexById;
    for (std::size_t i = 0; i < bills.size(); ++i)
    {
        bills[i].lines.clear();
        indexById[bills[i].billId] = i;
    }

    for (const BillDetailLine& line : allLines)
    {
        auto found = indexById.find(line.billId);
        if (found != indexById.end())
        {
            bills[found->second].lines.push_back(line);
        }
    }
    return bills;
}

bool BillingService::findBillById(const string& billId, BillHeader& outBill) const
{
    const vector<BillHeader> bills = loadBills();
    for (const BillHeader& bill : bills)
    {
        if (bill.billId == billId)
        {
            outBill = bill;
            return true;
        }
    }
    return false;
}

bool BillingService::findBillByOrderId(const string& orderId, BillHeader& outBill) const
{
    const vector<BillHeader> bills = loadBills();
    for (const BillHeader& bill : bills)
    {
        if (bill.orderId == orderId)
        {
            outBill = bill;
            return true;
        }
    }
    return false;
}

} // namespace inc3
