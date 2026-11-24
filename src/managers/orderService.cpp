#include "orderService.h"

#include <cmath>
#include <map>

#include "OrderManager.h"

namespace {

const double kEpsilon = 1e-9;

// productId -> index in `result`, keeping first-seen order.
void addDelta(std::vector<StockDelta> &result,
              std::map<int, std::size_t> &index,
              int productId,
              const std::string &barcode,
              double delta)
{
    auto found = index.find(productId);
    if (found == index.end())
    {
        index[productId] = result.size();
        StockDelta entry;
        entry.productId = productId;
        entry.barcode = barcode;
        entry.delta = delta;
        result.push_back(entry);
    }
    else
    {
        result[found->second].delta += delta;
    }
}

std::vector<StockDelta> dropZeros(std::vector<StockDelta> deltas)
{
    std::vector<StockDelta> kept;
    for (const StockDelta &entry : deltas)
    {
        if (std::fabs(entry.delta) > kEpsilon)
        {
            kept.push_back(entry);
        }
    }
    return kept;
}

std::vector<StockDelta> negated(std::vector<StockDelta> deltas)
{
    for (StockDelta &entry : deltas)
    {
        entry.delta = -entry.delta;
    }
    return deltas;
}

// Applies deltas; an empty list is a successful no-op.
bool applyStock(IStockAdjuster &stock, const std::vector<StockDelta> &deltas)
{
    return deltas.empty() || stock.adjustStock(deltas);
}

void stamp(std::vector<OrderDetail> &details, const std::string &orderId)
{
    for (OrderDetail &detail : details)
    {
        detail.orderId = orderId;
    }
}

} // namespace

const char *describe(OrderResult result)
{
    switch (result)
    {
    case OrderResult::Ok:            return "OK";
    case OrderResult::Invalid:       return "The order is not valid.";
    case OrderResult::NotFound:      return "Order not found.";
    case OrderResult::DuplicateId:   return "An order with this id already exists.";
    case OrderResult::NotEditable:   return "This order cannot be edited.";
    case OrderResult::NeedsApproval: return "Manager approval is required to edit this paid order.";
    case OrderResult::BadTransition: return "That status change is not allowed.";
    case OrderResult::StockFailed:   return "Not enough stock for this change.";
    case OrderResult::WriteFailed:   return "The order could not be saved.";
    }
    return "Unknown error.";
}

std::vector<StockDelta> OrderService::reserveDeltas(
    const std::vector<OrderDetail> &details)
{
    std::vector<StockDelta> result;
    std::map<int, std::size_t> index;
    for (const OrderDetail &detail : details)
    {
        addDelta(result, index, detail.productId, detail.productBarcode,
                 -detail.quantity);
    }
    return dropZeros(result);
}

std::vector<StockDelta> OrderService::releaseDeltas(
    const std::vector<OrderDetail> &details)
{
    return negated(reserveDeltas(details));
}

std::vector<StockDelta> OrderService::editDeltas(
    const std::vector<OrderDetail> &oldDetails,
    const std::vector<OrderDetail> &newDetails)
{
    std::vector<StockDelta> result;
    std::map<int, std::size_t> index;
    for (const OrderDetail &detail : oldDetails)
    {
        addDelta(result, index, detail.productId, detail.productBarcode,
                 detail.quantity);      // old quantity goes back...
    }
    for (const OrderDetail &detail : newDetails)
    {
        addDelta(result, index, detail.productId, detail.productBarcode,
                 -detail.quantity);     // ...new quantity is taken
    }
    return dropZeros(result);
}

OrderResult OrderService::saveNewOrder(Order &order,
                                       std::vector<OrderDetail> details,
                                       IStockAdjuster &stock,
                                       const OrderPaths &paths)
{
    if (order.status == "archived")
    {
        return OrderResult::BadTransition;
    }

    if (order.date.empty())
    {
        order.date = OrderManager::getCurrentDate();
    }
    if (order.time.empty())
    {
        order.time = OrderManager::getCurrentTime();
    }
    if (order.orderId.empty())
    {
        order.orderId = OrderManager::generateOrderId(order.date, paths.order);
        if (order.orderId.empty())
        {
            return OrderResult::Invalid;
        }
    }

    stamp(details, order.orderId);
    if (!OrderManager::validateOrder(order, details))
    {
        return OrderResult::Invalid;
    }

    Order existing;
    if (OrderManager::findOrder(order.orderId, existing, paths.order))
    {
        return OrderResult::DuplicateId;
    }

    const std::vector<StockDelta> reserve = reserveDeltas(details);
    if (!applyStock(stock, reserve))
    {
        return OrderResult::StockFailed;
    }

    if (!OrderManager::saveOrderTransaction(order, details, paths.order,
                                            paths.detail))
    {
        applyStock(stock, negated(reserve));
        return OrderResult::WriteFailed;
    }

    return OrderResult::Ok;
}

OrderResult OrderService::holdOrder(Order &order,
                                    std::vector<OrderDetail> details,
                                    IStockAdjuster &stock,
                                    const OrderPaths &paths)
{
    order.status = "on-hold";
    return saveNewOrder(order, std::move(details), stock, paths);
}

OrderResult OrderService::resumeOrder(const std::string &orderId,
                                      Order &order,
                                      std::vector<OrderDetail> &details,
                                      const OrderPaths &paths)
{
    Order stored;
    if (!OrderManager::findOrder(orderId, stored, paths.order))
    {
        return OrderResult::NotFound;
    }

    if (stored.status != "on-hold")
    {
        return OrderResult::BadTransition;
    }

    std::vector<OrderDetail> storedDetails =
        OrderManager::loadOrderDetails(orderId, paths.detail);
    if (storedDetails.empty())
    {
        return OrderResult::Invalid;
    }

    order = stored;
    details = storedDetails;
    return OrderResult::Ok;
}

OrderResult OrderService::editOrder(const Order &updated,
                                    std::vector<OrderDetail> details,
                                    const BillStamp &bill,
                                    const std::string &today,
                                    bool managerApproved,
                                    IStockAdjuster &stock,
                                    const OrderPaths &paths)
{
    Order stored;
    if (!OrderManager::findOrder(updated.orderId, stored, paths.order))
    {
        return OrderResult::NotFound;
    }

    if (!OrderPolicy::canEdit(stored, bill, today, managerApproved))
    {
        return OrderPolicy::requiresManagerApproval(stored, bill, today)
                   ? OrderResult::NeedsApproval
                   : OrderResult::NotEditable;
    }

    if (updated.status == "archived" ||
        !OrderPolicy::canTransition(stored.status, updated.status))
    {
        return OrderResult::BadTransition;
    }

    stamp(details, updated.orderId);
    if (!OrderManager::validateOrder(updated, details))
    {
        return OrderResult::Invalid;
    }

    const std::vector<OrderDetail> oldDetails =
        OrderManager::loadOrderDetails(stored.orderId, paths.detail);
    const std::vector<StockDelta> delta = editDeltas(oldDetails, details);

    if (!applyStock(stock, delta))
    {
        return OrderResult::StockFailed;
    }

    if (!OrderManager::updateOrderTransaction(updated, details, paths.order,
                                              paths.detail))
    {
        applyStock(stock, negated(delta));
        return OrderResult::WriteFailed;
    }

    return OrderResult::Ok;
}

OrderResult OrderService::archiveOrder(const std::string &orderId,
                                       IStockAdjuster &stock,
                                       const OrderPaths &paths)
{
    Order stored;
    if (!OrderManager::findOrder(orderId, stored, paths.order))
    {
        return OrderResult::NotFound;
    }

    if (!OrderPolicy::canTransition(stored.status, "archived") ||
        stored.status == "archived")
    {
        return OrderResult::BadTransition;
    }

    const std::vector<OrderDetail> details =
        OrderManager::loadOrderDetails(orderId, paths.detail);
    if (details.empty())
    {
        return OrderResult::Invalid;
    }

    const std::vector<StockDelta> release =
        OrderPolicy::isUnpaid(stored.status) ? releaseDeltas(details)
                                             : std::vector<StockDelta>();
    if (!applyStock(stock, release))
    {
        return OrderResult::StockFailed;
    }

    stored.status = "archived";
    if (!OrderManager::updateOrderTransaction(stored, details, paths.order,
                                              paths.detail))
    {
        applyStock(stock, negated(release));
        return OrderResult::WriteFailed;
    }

    return OrderResult::Ok;
}
