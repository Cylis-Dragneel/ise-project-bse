
#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <vector>

struct Order
{
    std::string orderId;
    std::string orderDate;
    std::string orderTime;

    std::string customerName;
    std::string customerContact;

    std::string status;
    double paymentAmount = 0.0;

    std::string paymentMode;
    std::string paymentMerchant;

    // Products included in this order
    std::vector<int> productIds;
};

#endif
