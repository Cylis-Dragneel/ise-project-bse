#include "BillingService.h"

#include "BillCalculator.h"
#include "OrderManager.h"
#include "catalog/Product.h"
#include "catalog/ProductCatalog.h"
#include "seasons/ISeasonProvider.h"
#include "util/DateUtils.h"

#include <map>
#include <stdexcept>

using std::string;
using std::vector;

BillingService::BillingService(const ProductCatalog& catalog,
                               std::shared_ptr<IBillRepository> billRepository,
                               std::shared_ptr<IBillDetailRepository> detailRepository,
                               const ISeasonProvider* seasons,
                               string detailFilePath)
    : catalog_(catalog),
      billRepository_(std::move(billRepository)),
      detailRepository_(std::move(detailRepository)),
      seasons_(seasons),
      detailFilePath_(std::move(detailFilePath))
{
    if (!billRepository_)
    {
        throw std::invalid_argument("BillingService needs a bill repository");
    }
    if (!detailRepository_)
    {
        throw std::invalid_argument("BillingService needs a bill detail repository");
    }
}

Bill BillingService::quote(const Order& order,
                           const vector<OrderDetail>& details,
                           const string& billDate,
                           double specialDiscountPercent) const
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

    const string date = billDate.empty() ? DateUtils::today() : billDate;

    Bill bill;
    bill.orderId = order.orderId;
    bill.customerName = order.customerName;
    bill.paymentStatus = order.status;
    bill.amountReceived = order.paymentAmount;
    bill.specialDiscountPercent = specialDiscountPercent;
    bill.billDate = date;

    for (const OrderDetail& detail : details)
    {
        const Product* product = catalog_.findByBarcode(detail.productBarcode);
        bill.lines.push_back(BillCalculator::makeLine("", detail, *product));
    }

    const double seasonalPercent = seasons_ == nullptr ? 0.0 : seasons_->activePercent(date);
    BillCalculator::calculate(bill, seasonalPercent);
    return bill;
}

Bill BillingService::generateBill(const Order& order,
                                  const vector<OrderDetail>& details,
                                  double specialDiscountPercent)
{
    const string billDate = DateUtils::today();
    Bill bill = quote(order, details, billDate, specialDiscountPercent);

    if (bill.amountReceived < bill.totalPayable)
    {
        throw std::invalid_argument(
            "amount received is less than the total payable for order " + order.orderId);
    }

    // Only a persisted bill gets an id.
    bill.billId = BillCalculator::nextBillId(billRepository_->loadAll());
    bill.billTime = DateUtils::nowTime();
    for (BillLine& line : bill.lines)
    {
        line.billId = bill.billId;
    }

    // Persist the detail rows first, then the header. If the header write
    // fails, remove the detail rows that were just added so the two files stay
    // consistent.
    const vector<BillLine> oldDetails = detailRepository_->loadAll();
    vector<BillLine> newDetails = oldDetails;
    newDetails.insert(newDetails.end(), bill.lines.begin(), bill.lines.end());

    detailRepository_->saveAll(newDetails);
    try
    {
        const vector<Bill> oldBills = billRepository_->loadAll();
        vector<Bill> newBills = oldBills;
        newBills.push_back(bill);
        billRepository_->saveAll(newBills);
    }
    catch (...)
    {
        try
        {
            detailRepository_->saveAll(oldDetails);
        }
        catch (...)
        {
            // Keep the original exception; the orphan rows can be cleaned up by
            // loading the bills and dropping unknown detail rows.
        }
        throw;
    }

    return bill;
}

Bill BillingService::generateBill(const Order& order, double specialDiscountPercent)
{
    const vector<OrderDetail> details =
        OrderManager::loadOrderDetails(order.orderId, detailFilePath_);

    if (details.empty())
    {
        throw std::invalid_argument("order " + order.orderId +
                                    " has no detail rows in " + detailFilePath_);
    }

    return generateBill(order, details, specialDiscountPercent);
}

void BillingService::replaceBill(const Bill& bill)
{
    if (bill.billId.empty())
    {
        throw std::invalid_argument("replaceBill needs a bill id");
    }

    vector<Bill> bills = billRepository_->loadAll();
    bool found = false;
    for (Bill& existing : bills)
    {
        if (existing.billId == bill.billId)
        {
            existing = bill;
            found = true;
            break;
        }
    }
    if (!found)
    {
        throw std::invalid_argument("unknown bill id: " + bill.billId);
    }

    vector<BillLine> details = detailRepository_->loadAll();
    vector<BillLine> kept;
    for (const BillLine& line : details)
    {
        if (line.billId != bill.billId) kept.push_back(line);
    }
    kept.insert(kept.end(), bill.lines.begin(), bill.lines.end());

    // Keep the old detail rows so a failed header write can be undone.
    const vector<BillLine> oldDetails = details;
    detailRepository_->saveAll(kept);
    try
    {
        billRepository_->saveAll(bills);
    }
    catch (...)
    {
        try
        {
            detailRepository_->saveAll(oldDetails);
        }
        catch (...)
        {
        }
        throw;
    }
}

vector<Bill> BillingService::loadBills()
{
    vector<Bill> bills = billRepository_->loadAll();

    std::map<string, std::size_t> indexById;
    for (std::size_t i = 0; i < bills.size(); ++i)
    {
        if (!indexById.insert(std::make_pair(bills[i].billId, i)).second)
        {
            throw std::invalid_argument("duplicate bill id: " + bills[i].billId);
        }
    }

    const vector<BillLine> details = detailRepository_->loadAll();
    for (const BillLine& line : details)
    {
        auto found = indexById.find(line.billId);
        if (found == indexById.end())
        {
            throw std::invalid_argument("bill detail refers to unknown bill id: " +
                                        line.billId);
        }

        BillLine display = line;
        display.lineTotal = BillCalculator::roundMoney(
            BillCalculator::lineSubtotal(display) - display.discountTotal);
        bills[found->second].lines.push_back(display);
    }

    return bills;
}
