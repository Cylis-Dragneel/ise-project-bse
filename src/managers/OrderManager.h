#ifndef ORDERMANAGER_H
#define ORDERMANAGER_H

#include <string>
#include <vector>
#include "../models/Order.h"

class OrderManager {
public:
    static std::string getCurrentDate();
    static std::string getCurrentTime();
    static std::string generateOrderId(const std::string& date = "");
    static bool validateOrder(const Order& order, const std::vector<OrderDetail>& details);
    static bool saveOrderTransaction(const Order& order, const std::vector<OrderDetail>& details);
    static std::vector<Order> loadOrders();
    static std::vector<OrderDetail> loadOrderDetails(const std::string& orderId);
    static bool findOrder(const std::string& orderId, Order& result);
};

#endif