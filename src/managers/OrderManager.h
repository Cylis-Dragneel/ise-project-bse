#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include <string>
#include <vector>
#include "../models/Order.h"

class OrderManager {
public:
    static std::string getCurrentDate();
    static std::string getCurrentTime();

    // Generates XXXXMMDD by finding the highest sequence already used for
    // `date` in orderFilePath. Returns "" for an invalid date or a full
    // 9999 sequence.
    static std::string generateOrderId(const std::string& date = "",
                                       const std::string& orderFilePath = "Order.txt");

    static bool validateOrder(const Order& order,
                              const std::vector<OrderDetail>& details);

    // Appends one order and its detail rows. Both files are opened before any
    // write, and a duplicate order id is rejected.
    static bool saveOrderTransaction(const Order& order,
                                     const std::vector<OrderDetail>& details,
                                     const std::string& orderFilePath = "Order.txt",
                                     const std::string& detailFilePath = "OrderDetail.txt");

    static std::vector<Order> loadOrders(const std::string& orderFilePath = "Order.txt");
    static std::vector<OrderDetail> loadOrderDetails(
        const std::string& orderId,
        const std::string& detailFilePath = "OrderDetail.txt");
    static bool findOrder(const std::string& orderId,
                          Order& result,
                          const std::string& orderFilePath = "Order.txt");

    // Replaces an EXISTING order row and all of its detail rows in place
    // (file order is preserved). Returns false for an invalid order, an
    // unknown order id, or an unreadable file; in every failure case both
    // files are left as they were. Both new files are staged as .tmp files
    // before either original is replaced.
    static bool updateOrderTransaction(
        const Order &order,
        const std::vector<OrderDetail> &details,
        const std::string &orderFilePath = "Order.txt",
        const std::string &detailFilePath = "OrderDetail.txt");

    // Loads only the orders whose status equals statusFilter ("" = all).
    // Named separately so it cannot be confused with the file-path overload.
    static std::vector<Order> loadOrdersByStatus(
        const std::string &statusFilter,
        const std::string &orderFilePath = "Order.txt");
};

#endif
