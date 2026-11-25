#include "BillingService.h"

#include "BillCalculator.h"
#include "BillRecordCodec.h"
#include "FileBillRepository.h"
#include "OrderManager.h"
#include "catalog/ProductCatalog.h"
#include "seasons/SeasonService.h"

#include <map>
#include <stdexcept>
#include <unordered_map>

using std::string;
using std::vector;

BillingService::BillingService(const ProductCatalog& catalog,
                               std::shared_ptr<IBillRepository> billRepository,
                               std::shared_ptr<IBillDetailRepository> detailRepository,
                               std::shared_ptr<ISeasonProvider> seasonProvider,
                               string orderDetailFilePath)
    : catalog_(catalog),
      billRepository_(std::move(billRepository)),
      detailRepository_(detailRepository
                            ? std::move(detailRepository)
                            : std::make_shared<FileBillDetailRepository>("BillDetail.txt")),
      seasonProvider_(std::move(seasonProvider)),
      rawSeasonService_(nullptr),
      orderDetailFilePath_(std::move(orderDetailFilePath))
{
    if (!billRepository_ || !detailRepository_)
    {
        throw std::invalid_argument("BillingService requires bill and detail repositories");
    }
}

BillingService::BillingService(const ProductCatalog& catalog,
                               std::shared_ptr<IBillRepository> billRepository,
                               std::shared_ptr<IBillDetailRepository> detailRepository,
                               SeasonService* seasonService,
                               string orderDetailFilePath)
    : catalog_(catalog),
      billRepository_(std::move(billRepository)),
      detailRepository_(detailRepository
                            ? std::move(detailRepository)
                            : std::make_shared<FileBillDetailRepository>("BillDetail.txt")),
      seasonProvider_(nullptr),
      rawSeasonService_(seasonService),
      orderDetailFilePath_(std::move(orderDetailFilePath))
{
    if (!billRepository_ || !detailRepository_)
    {
        throw std::invalid_argument("BillingService requires bill and detail repositories");
    }
}

BillingService::BillingService(const ProductCatalog& catalog,
                               std::shared_ptr<IBillRepository> billRepository,
                               SeasonService* seasonService,
                               string orderDetailFilePath,
                               std::shared_ptr<IBillDetailRepository> detailRepository)
    : catalog_(catalog),
      billRepository_(std::move(billRepository)),
      detailRepository_(detailRepository
                            ? std::move(detailRepository)
                            : std::make_shared<FileBillDetailRepository>("BillDetail.txt")),
      seasonProvider_(nullptr),
      rawSeasonService_(seasonService),
      orderDetailFilePath_(std::move(orderDetailFilePath))
{
    if (!billRepository_ || !detailRepository_)
    {
        throw std::invalid_argument("BillingService requires bill and detail repositories");
    }
}

double BillingService::resolveSeasonPercent(const string& effectiveDate) const
{
    if (seasonProvider_)
    {
        return seasonProvider_->activePercent(effectiveDate);
    }
    if (rawSeasonService_ != nullptr)
    {
        return rawSeasonService_->activePercent(effectiveDate);
    }
    return 0.0;
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

Bill BillingService::quote(const Order& order,
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

    const double seasonPct = resolveSeasonPercent(effectiveDate);

    Bill bill;
    bill.orderId = order.orderId;
    bill.customerName = BillRecordCodec::sanitizeCustomerName(order.customerName);
    bill.paymentStatus = order.status;
    bill.billDate = effectiveDate;
    bill.billTime = effectiveTime;

    BillCalculator::calculate(bill, details, products, seasonPct,
                            specialDiscountPercent, amountReceived, false);
    return bill;
}

Bill BillingService::generateBill(const Order& order,
                                  const vector<OrderDetail>& details,
                                  double specialDiscountPercent,
                                  double amountReceived,
                                  const string& billDate,
                                  const string& billTime)
{
    const vector<Product> products = resolveProducts(order, details, true);

    const string effectiveDate = billDate.empty()
                                     ? (order.date.empty() ? OrderManager::getCurrentDate() : order.date)
                                     : billDate;
    const string effectiveTime = billTime.empty()
                                     ? (order.time.empty() ? OrderManager::getCurrentTime() : order.time)
                                     : billTime;

    const double seasonPct = resolveSeasonPercent(effectiveDate);

    Bill bill;
    bill.orderId = order.orderId;
    bill.customerName = BillRecordCodec::sanitizeCustomerName(order.customerName);
    bill.paymentStatus = order.status;
    bill.billDate = effectiveDate;
    bill.billTime = effectiveTime;

    BillCalculator::calculate(bill, details, products, seasonPct,
                            specialDiscountPercent, amountReceived, true);

    bill.billId = BillCalculator::nextBillId(billRepository_->loadAll());
    for (BillLine& line : bill.lines)
    {
        line.billId = bill.billId;
    }

    const vector<BillLine> previousDetails = detailRepository_->loadAll();

    detailRepository_->append(bill.lines);
    try
    {
        billRepository_->append(bill);
    }
    catch (...)
    {
        detailRepository_->saveAll(previousDetails);
        throw;
    }

    return bill;
}

Bill BillingService::generateBill(const Order& order,
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

Bill BillingService::updateExistingBill(const string& existingBillId,
                                        const Order& updatedOrder,
                                        const vector<OrderDetail>& updatedDetails,
                                        double specialDiscountPercent,
                                        double amountReceived,
                                        const string& newBillDate,
                                        const string& newBillTime)
{
    vector<Bill> allBills = billRepository_->loadAll();
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

    const vector<Product> products = resolveProducts(updatedOrder, updatedDetails, false);

    const string effectiveDate = newBillDate.empty() ? OrderManager::getCurrentDate() : newBillDate;
    const string effectiveTime = newBillTime.empty() ? OrderManager::getCurrentTime() : newBillTime;
    const double seasonPct = resolveSeasonPercent(effectiveDate);

    Bill updatedBill;
    updatedBill.billId = existingBillId;
    updatedBill.orderId = updatedOrder.orderId;
    updatedBill.customerName = BillRecordCodec::sanitizeCustomerName(updatedOrder.customerName);
    updatedBill.paymentStatus = updatedOrder.status;
    updatedBill.billDate = effectiveDate;
    updatedBill.billTime = effectiveTime;

    BillCalculator::calculate(updatedBill, updatedDetails, products, seasonPct,
                            specialDiscountPercent, amountReceived, true);

    const vector<BillLine> previousDetails = detailRepository_->loadAll();
    vector<BillLine> replacedDetails;
    replacedDetails.reserve(previousDetails.size() + updatedBill.lines.size());

    for (const BillLine& line : previousDetails)
    {
        if (line.billId != existingBillId)
        {
            replacedDetails.push_back(line);
        }
    }
    replacedDetails.insert(replacedDetails.end(), updatedBill.lines.begin(), updatedBill.lines.end());

    detailRepository_->saveAll(replacedDetails);
    try
    {
        allBills[targetIdx] = updatedBill;
        billRepository_->saveAll(allBills);
    }
    catch (...)
    {
        detailRepository_->saveAll(previousDetails);
        throw;
    }

    return updatedBill;
}

vector<Bill> BillingService::loadBills() const
{
    vector<Bill> bills = billRepository_->loadAll();
    const vector<BillLine> allLines = detailRepository_->loadAll();

    std::unordered_map<string, std::size_t> indexById;
    for (std::size_t i = 0; i < bills.size(); ++i)
    {
        bills[i].lines.clear();
        indexById[bills[i].billId] = i;
    }

    for (const BillLine& line : allLines)
    {
        auto found = indexById.find(line.billId);
        if (found == indexById.end())
        {
            throw std::invalid_argument("orphan bill detail row references unknown billId: '" +
                                        line.billId + "'");
        }
        bills[found->second].lines.push_back(line);
    }
    return bills;
}

bool BillingService::findBillById(const string& billId, Bill& outBill) const
{
    const vector<Bill> bills = loadBills();
    for (const Bill& bill : bills)
    {
        if (bill.billId == billId)
        {
            outBill = bill;
            return true;
        }
    }
    return false;
}

bool BillingService::findBillByOrderId(const string& orderId, Bill& outBill) const
{
    const vector<Bill> bills = loadBills();
    for (const Bill& bill : bills)
    {
        if (bill.orderId == orderId)
        {
            outBill = bill;
            return true;
        }
    }
    return false;
}