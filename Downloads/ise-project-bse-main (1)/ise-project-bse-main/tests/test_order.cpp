#include "test_framework.h"
#include "../src/managers/OrderManager.h"
#include "../models/Order.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {
const char* orderPath = "Order.txt";
const char* detailPath = "OrderDetail.txt";

void removeTestFiles()
{
    std::remove(orderPath);
    std::remove(detailPath);
}

std::string readFile(const char* path)
{
    std::ifstream file(path);
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

Order makeOrder()
{
    Order order;
    order.orderId = "00011118";       // Seq 0001 + MMDD (1118)
    order.date = "2026-11-18";
    order.time = "19:30:00";
    order.customerName = "Ali Khan";
    order.contact = "0300 1234567";
    order.merchant = "Jazz Cash";
    order.totalAmount = 1250.5;
    order.paymentAmount = 1250.5;
    order.status = "Completed";        // Valid options: Pending, Completed, Cancelled
    order.paymentMode = "Online";      // Valid options: Cash, Card, Online
    return order;
}

std::vector<OrderDetail> makeDetails()
{
    OrderDetail detail;
    detail.orderId = "00011118";
    detail.productId = 27;
    detail.quantity = 4.5;             // Supports fractional weights
   // detail.unitPrice = 200.0;
    //detail.subtotal = 900.0;
    return std::vector<OrderDetail>(1, detail);
}
}

TEST_CASE(order_id_uses_order_date_field_only)
{
    removeTestFiles();
    {
        std::ofstream file(orderPath);
        // Format layout check based on OrderManager parsing rules
        file << "00012026-11-18 19:30:00 Ali_Khan 0300_1234567 Jazz_Cash 1250.5 1250.5 Completed Online #\n";
        file << "00021118 2026-11-18 10:00:00 Customer - - 0 0 Pending Cash #\n";
        file << "00031117 2026-11-17 11:00:00 Customer - - 0 0 Pending Cash #\n";
    }

    // generateOrderId uses static method and checks Order.txt matching date lines
    std::string newId = OrderManager::generateOrderId("2026-11-18");
    CHECK_EQ(newId, std::string("00031118")); // max seq is 2, becomes 0003 + 1118

    std::string nextDayId = OrderManager::generateOrderId("2026-11-19");
    CHECK_EQ(nextDayId, std::string("00011119")); // seq starts at 1 for new date + 1119

    std::string invalidId = OrderManager::generateOrderId("bad-date");
    CHECK_EQ(invalidId, std::string(""));

    removeTestFiles();
}

TEST_CASE(order_transaction_writes_exact_record_layout)
{
    removeTestFiles();
    Order order = makeOrder();

    CHECK(OrderManager::saveOrderTransaction(order, makeDetails()));

    // Verify file contents contain the expected exact transaction record and suffix marker
    std::string orderContent = readFile(orderPath);
    CHECK(orderContent.find("00011118 2026-11-18 19:30:00 Ali_Khan") != std::string::npos);
    CHECK(orderContent.find("Completed Online #") != std::string::npos);

    std::string detailContent = readFile(detailPath);
    CHECK(detailContent.find("00011118 27 4.500000 200.000000 900.000000 #") != std::string::npos);

    removeTestFiles();
}

TEST_CASE(optional_fields_are_written_as_hyphen)
{
    removeTestFiles();
    Order order = makeOrder();
    order.contact = "";
    order.merchant = "";

    CHECK(OrderManager::saveOrderTransaction(order, makeDetails()));

    std::string fileContent = readFile(orderPath);
    // Empty fields get sanitized to "-"
    CHECK(fileContent.find("- -") != std::string::npos);

    removeTestFiles();
}

TEST_CASE(invalid_order_and_detail_records_are_rejected)
{
    removeTestFiles();
    Order order = makeOrder();

    // Invalid Status
    order.status = "Incomplete";
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails()));

    // Invalid Payment Mode
    order = makeOrder();
    order.paymentMode = "Crypto";
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails()));

    // Negative Amount
    order = makeOrder();
    order.totalAmount = -5.0;
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails()));

    // Order ID mismatch with details
    order = makeOrder();
    std::vector<OrderDetail> details = makeDetails();
    details[0].orderId = "99991118";
    CHECK(!OrderManager::saveOrderTransaction(order, details));

    // Zero Quantity
    details = makeDetails();
    details[0].quantity = 0.0;
    CHECK(!OrderManager::saveOrderTransaction(order, details));

    // Subtotal Mismatch
    details = makeDetails();
   // details[0].subtotal = 5000.0; // Incorrect math against unitPrice * quantity
    CHECK(!OrderManager::saveOrderTransaction(order, details));

    CHECK_EQ(readFile(orderPath), std::string(""));
    CHECK_EQ(readFile(detailPath), std::string(""));
    removeTestFiles();
}