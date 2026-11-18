#ifndef ORDER_H
#define ORDER_H

#include <string>

// Order.txt record:
//   orderId date time customer contact status paymentAmount paymentMode merchant #
//
// The order total is derived from the detail rows and Product.txt; it is not
// part of the Order.txt record.
struct Order {
    std::string orderId;      // XXXXMMDD: 4-digit daily sequence + MMDD
    std::string date;         // YYYY-MM-DD
    std::string time;         // HH:MM:SS
    std::string customerName;
    std::string contact;
    std::string status = "in-progress";   // in-progress, on-hold, paid, archived
    double paymentAmount = 0;
    std::string paymentMode = "cash";     // cash, card, wallet
    std::string merchant;
};

// OrderDetail.txt record:
//   orderId productId productBarcode quantity #
//
// Unit price and subtotal belong to Bill.txt; the order stores the product
// reference and quantity only.
struct OrderDetail {
    std::string orderId;
    int productId = 0;
    std::string productBarcode;
    double quantity = 0;      // double for weights like 4.5 kg
};

#endif
