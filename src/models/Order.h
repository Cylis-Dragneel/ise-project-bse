#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <vector>

struct OrderDetail {
    std::string orderId;
    std::string productId;
    std::string productBarcode;
    int quantity;
};

struct Order {
    std::string orderId;          // 8 digits: XXXXMMDD (4-digit daily sequence + Date)
    std::string orderDate;        // Format: YYYY-MM-DD
    std::string orderTime;        // Format: HH:MM:SS
    std::string customerName;
    std::string customerContact;  
    std::string status;           
    double paymentAmount;
    std::string paymentMode;      
    std::string paymentMerchant;  
};

#endif