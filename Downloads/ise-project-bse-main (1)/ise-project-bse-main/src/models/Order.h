#ifndef ORDER_H
#define ORDER_H

#include <string>

struct Order {
    std::string orderId;      // XXXXMMDD (8 digit)
    std::string date;         // YYYY-MM-DD
    std::string time;         // HH:MM:SS
    std::string customerName;
    std::string contact;
    std::string merchant;
    double totalAmount = 0;
    double paymentAmount = 0;
    std::string status = "in-progress";   // in-progress, on-hold, paid, archived
    std::string paymentMode = "cash";     // cash, card, wallet
};

struct OrderDetail {
    std::string orderId;
    int productId = 0;
    std::string barcode;
    int quantity = 0;
};

#endif