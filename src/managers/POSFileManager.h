
#ifndef POS_FILE_MANAGER_H
#define POS_FILE_MANAGER_H

#include <vector>
#include <string>

#include "../models/Product.h"
#include "../models/Order.h"
#include "../models/Bill.h"

class POSFileManager
{
public:
    // Read all products from Product.txt
    static std::vector<Product> loadProducts();

    // Save a new order in Order.txt
    static bool saveOrder(const Order& order);

    // Save order product details in OrderDetail.txt
    static bool saveOrderDetails(
        const std::string& orderId,
        const std::vector<int>& productIds
    );

    // Save a bill in Bill.txt
    static bool saveBill(const Bill& bill);
};

#endif
