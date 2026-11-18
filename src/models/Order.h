#ifndef ORDER_H
#define ORDER_H

#include <string>

struct Order {
    std::string orderId;      // XXXXMMDD
    std::string date;         // YYYY-MM-DD
    std::string time;         // HH:MM:SS
    std::string customerName;
    std::string contact;
    std::string merchant;
    double totalAmount = 0;
    double paymentAmount = 0;
    std::string status = "Pending";   // Pending, Completed, Cancelled
    std::string paymentMode = "Cash"; // Cash, Card, Online
};

struct OrderDetail {
    std::string orderId;
    int productId = 0;
    double quantity = 0;      // double for weights like 4.5 kg
    double unitPrice = 0;
    double subtotal = 0;
};

#endif