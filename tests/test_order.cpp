#include "test_framework.h"
#include "../src/managers/OrderManager.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {
const char* orderPath = "test_Order.txt";
const char* detailPath = "test_OrderDetail.txt";

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
    order.orderId = "00011118";
    order.orderDate = "2026-11-18";
    order.orderTime = "19:30:00";
    order.customerName = "Ali Khan";
    order.customerContact = "0300 1234567";
    order.status = "paid";
    order.paymentAmount = 1250.5;
    order.paymentMode = "wallet";
    order.paymentMerchant = "Jazz Cash";
    return order;
}

std::vector<OrderDetail> makeDetails()
{
    OrderDetail detail;
    detail.orderId = "00011118";
    detail.productId = 27;
    detail.productBarcode = "8964001234567";
    detail.quantity = 4.5;
    return std::vector<OrderDetail>(1, detail);
}
}

TEST_CASE(order_id_uses_order_date_field_only)
{
    removeTestFiles();
    {
        std::ofstream file(orderPath);
        file << "00011118 2026-11-18 10:00:00 03001118 paid 0 cash - #\n";
        file << "00021118 2026-11-18 11:00:00 Customer 03001118 paid 0 cash - #\n";
        file << "00031117 2026-11-17 11:00:00 Customer 03001118 paid 0 cash - #\n";
    }

    OrderManager manager(orderPath, detailPath);
    CHECK_EQ(manager.generateOrderId("2026-11-18"), std::string("00031118"));
    CHECK_EQ(manager.generateOrderId("2026-11-19"), std::string("00011119"));
    CHECK_EQ(manager.generateOrderId("bad-date"), std::string(""));
    removeTestFiles();
}

TEST_CASE(order_transaction_writes_exact_record_layout)
{
    removeTestFiles();
    OrderManager manager(orderPath, detailPath);
    Order order = makeOrder();

    CHECK(manager.saveOrderTransaction(order, makeDetails()));

    CHECK_EQ(readFile(orderPath),
             std::string("00011118 2026-11-18 19:30:00 Ali_Khan "
                         "0300_1234567 paid 1250.5 wallet Jazz_Cash #\n"));
    CHECK_EQ(readFile(detailPath),
             std::string("00011118 27 8964001234567 4.5 #\n"));
    removeTestFiles();
}

TEST_CASE(optional_fields_are_written_as_hyphen)
{
    removeTestFiles();
    OrderManager manager(orderPath, detailPath);
    Order order = makeOrder();
    order.customerContact = "";
    order.paymentMerchant = "";

    CHECK(manager.saveOrderTransaction(order, makeDetails()));
    CHECK_EQ(readFile(orderPath),
             std::string("00011118 2026-11-18 19:30:00 Ali_Khan "
                         "- paid 1250.5 wallet - #\n"));
    removeTestFiles();
}

TEST_CASE(detail_path_failure_does_not_write_order_record)
{
    removeTestFiles();
    OrderManager manager(orderPath, "missing_test_folder/OrderDetail.txt");

    CHECK(!manager.saveOrderTransaction(makeOrder(), makeDetails()));
    CHECK_EQ(readFile(orderPath), std::string(""));
    removeTestFiles();
}

TEST_CASE(invalid_order_and_detail_records_are_rejected)
{
    removeTestFiles();
    OrderManager manager(orderPath, detailPath);
    Order order = makeOrder();

    order.status = "completed";
    CHECK(!manager.saveOrderTransaction(order, makeDetails()));

    order = makeOrder();
    order.paymentMode = "bank";
    CHECK(!manager.saveOrderTransaction(order, makeDetails()));

    order = makeOrder();
    order.paymentAmount = -1.0;
    CHECK(!manager.saveOrderTransaction(order, makeDetails()));

    order = makeOrder();
    std::vector<OrderDetail> details = makeDetails();
    details[0].orderId = "00021118";
    CHECK(!manager.saveOrderTransaction(order, details));

    details = makeDetails();
    details[0].quantity = 0.0;
    CHECK(!manager.saveOrderTransaction(order, details));

    details = makeDetails();
    details[0].quantity = -2.5;
    CHECK(!manager.saveOrderTransaction(order, details));

    CHECK_EQ(readFile(orderPath), std::string(""));
    CHECK_EQ(readFile(detailPath), std::string(""));
    removeTestFiles();
}
