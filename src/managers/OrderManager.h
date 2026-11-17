#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include "../models/Order.h"
#include <string>
#include <vector>

class OrderManager {
public:
    /**
     * Generates an 8-digit Order ID in XXXXMMDD format by reading existing records
     * in Order.txt to calculate the correct daily sequence count (XXXX)
     */
    static std::string generateOrderId(int dayOfMonth, const std::string& orderFilePath = "Order.txt");

    /**
     * Appends an order to Order.txt and its line items to OrderDetail.txt
     * Format: space-separated fields ending strictly with '#' and a newline
     */
    static bool saveOrderTransaction(const Order& order, 
                                     const std::vector<OrderDetail>& details,
                                     const std::string& orderFilePath = "Order.txt",
                                     const std::string& detailFilePath = "OrderDetail.txt");
};

#endif