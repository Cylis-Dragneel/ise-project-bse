#ifndef ORDER_SERVICE_H
#define ORDER_SERVICE_H

#include <string>
#include <vector>
#include "../models/Order.h"
#include "orderPolicy.h"

// One stock movement for a product. Positive = return to stock,
// negative = deduct. Keyed by productId (what OrderDetail stores);
// `barcode` is carried along for the adapter's convenience.
struct StockDelta {
    int productId = 0;
    std::string barcode;
    double delta = 0;
};

// Implemented over Person A's ProductCatalog::adjustStock(...). Contract:
// all-or-nothing. Returns false (and changes nothing) if any product is
// unknown or would go below zero.
class IStockAdjuster {
public:
    virtual ~IStockAdjuster() = default;
    virtual bool adjustStock(const std::vector<StockDelta> &deltas) = 0;
};

struct OrderPaths {
    std::string order = "Order.txt";
    std::string detail = "OrderDetail.txt";
};

enum class OrderResult {
    Ok,
    Invalid,         // failed validateOrder / empty cart / no id available
    NotFound,        // unknown order id
    DuplicateId,     // caller-supplied id already exists
    NotEditable,     // archived order
    NeedsApproval,   // paid order older than 7 days, no manager approval
    BadTransition,   // status change not allowed by OrderPolicy
    StockFailed,     // not enough stock / unknown product; nothing changed
    WriteFailed      // file write failed; stock change was rolled back
};

const char *describe(OrderResult result);

// Orders + stock, Qt-free. Stock is reserved when an order is SAVED
// (including on-hold), edits move only the difference, and archiving an
// unpaid order returns its stock. Each operation adjusts stock first, then
// writes the files, and reverses the stock change if the write fails.
class OrderService {
public:
    // --- pure helpers (same product on several rows is summed) ---
    static std::vector<StockDelta> reserveDeltas(
        const std::vector<OrderDetail> &details);          // all negative
    static std::vector<StockDelta> releaseDeltas(
        const std::vector<OrderDetail> &details);          // all positive
    static std::vector<StockDelta> editDeltas(
        const std::vector<OrderDetail> &oldDetails,
        const std::vector<OrderDetail> &newDetails);       // old - new

    // --- operations ---

    // New order. Empty orderId/date/time are filled in (id from
    // OrderManager::generateOrderId); `order` is updated so the caller sees
    // what was saved. Detail rows get the order id. Any status except
    // archived is accepted.
    static OrderResult saveNewOrder(Order &order,
                                    std::vector<OrderDetail> details,
                                    IStockAdjuster &stock,
                                    const OrderPaths &paths = OrderPaths());

    // One-click hold: same as saveNewOrder with status forced to on-hold.
    // Customer name and payment fields may be left at their defaults.
    static OrderResult holdOrder(Order &order,
                                 std::vector<OrderDetail> details,
                                 IStockAdjuster &stock,
                                 const OrderPaths &paths = OrderPaths());

    // Loads an on-hold order and its items for the cart. Writes nothing; the
    // order stays on-hold (reserved) until it is saved again via editOrder.
    static OrderResult resumeOrder(const std::string &orderId,
                                   Order &order,
                                   std::vector<OrderDetail> &details,
                                   const OrderPaths &paths = OrderPaths());

    // Replaces an existing order. The policy is checked against the STORED
    // order. `today` is YYYY-MM-DD; `bill` is the order's bill stamp;
    // `managerApproved` is the UI's per-order approval. Use archiveOrder to
    // archive (editing to "archived" is rejected).
    static OrderResult editOrder(const Order &updated,
                                 std::vector<OrderDetail> details,
                                 const BillStamp &bill,
                                 const std::string &today,
                                 bool managerApproved,
                                 IStockAdjuster &stock,
                                 const OrderPaths &paths = OrderPaths());

    // Sets status to archived. An unpaid order's stock is returned; a paid
    // order's stock is left alone (it was sold).
    static OrderResult archiveOrder(const std::string &orderId,
                                    IStockAdjuster &stock,
                                    const OrderPaths &paths = OrderPaths());
};

#endif
