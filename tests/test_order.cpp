#include "test_framework.h"

#include "Order.h"
#include "OrderManager.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

const char *orderPath = "test_order_tmp.txt";
const char *detailPath = "test_order_detail_tmp.txt";

void removeTestFiles()
{
    std::remove(orderPath);
    std::remove(detailPath);
}

std::string readFile(const char *path)
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
    order.status = "paid";
    order.paymentAmount = 1250.5;
    order.paymentMode = "card";
    order.merchant = "Jazz Cash";
    return order;
}

std::vector<OrderDetail> makeDetails()
{
    OrderDetail detail;
    detail.orderId = "00011118";
    detail.productId = 27;
    detail.productBarcode = "428647";
    detail.quantity = 4.5;
    return std::vector<OrderDetail>(1, detail);
}

} // namespace

TEST_CASE(order_id_uses_order_date_field_only)
{
    removeTestFiles();
    {
        std::ofstream file(orderPath);
        // The 1117 line contains "1118" in its contact field, so a whole-line
        // substring search would count it. The 1118 line has a gap at
        // sequence 3, so a count-based implementation would reuse 0003.
        file << "00011117 2026-11-17 10:00:00 C 1118 - in-progress 0 cash - #\n";
        file << "00031118 2026-11-18 10:00:00 C - - in-progress 0 cash - #\n";
    }

    CHECK_EQ(OrderManager::generateOrderId("2026-11-18", orderPath),
             std::string("00041118"));
    CHECK_EQ(OrderManager::generateOrderId("2026-11-19", orderPath),
             std::string("00011119"));
    CHECK_EQ(OrderManager::generateOrderId("bad-date", orderPath),
             std::string(""));

    removeTestFiles();
}

TEST_CASE(order_transaction_writes_exact_record_layout)
{
    removeTestFiles();
    const Order order = makeOrder();

    CHECK(OrderManager::saveOrderTransaction(order, makeDetails(), orderPath,
                                             detailPath));

    CHECK_EQ(readFile(orderPath),
             std::string("00011118 2026-11-18 19:30:00 Ali_Khan "
                         "0300_1234567 paid 1250.5 card Jazz_Cash #\n"));
    CHECK_EQ(readFile(detailPath),
             std::string("00011118 27 428647 4.5 #\n"));

    removeTestFiles();
}

TEST_CASE(optional_fields_are_written_as_hyphen)
{
    removeTestFiles();
    Order order = makeOrder();
    order.contact.clear();
    order.merchant.clear();

    CHECK(OrderManager::saveOrderTransaction(order, makeDetails(), orderPath,
                                             detailPath));

    CHECK_EQ(readFile(orderPath),
             std::string("00011118 2026-11-18 19:30:00 Ali_Khan - paid "
                         "1250.5 card - #\n"));

    removeTestFiles();
}

TEST_CASE(invalid_order_and_detail_records_are_rejected)
{
    removeTestFiles();
    Order order = makeOrder();

    // Invalid status.
    order.status = "Incomplete";
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails(), orderPath,
                                              detailPath));

    // Invalid payment mode.
    order = makeOrder();
    order.paymentMode = "Crypto";
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails(), orderPath,
                                              detailPath));

    // Negative payment amount.
    order = makeOrder();
    order.paymentAmount = -5.0;
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails(), orderPath,
                                              detailPath));

    // Order ID mismatch with details.
    order = makeOrder();
    std::vector<OrderDetail> details = makeDetails();
    details[0].orderId = "99991118";
    CHECK(!OrderManager::saveOrderTransaction(order, details, orderPath,
                                              detailPath));

    // Zero quantity.
    details = makeDetails();
    details[0].quantity = 0.0;
    CHECK(!OrderManager::saveOrderTransaction(order, details, orderPath,
                                              detailPath));

    // Invalid barcode (leading zero).
    details = makeDetails();
    details[0].productBarcode = "012345";
    CHECK(!OrderManager::saveOrderTransaction(order, details, orderPath,
                                              detailPath));

    // Invalid calendar date.
    order = makeOrder();
    order.date = "2026-13-45";
    order.orderId = "00011345";
    details = makeDetails();
    details[0].orderId = "00011345";
    CHECK(!OrderManager::saveOrderTransaction(order, details, orderPath,
                                              detailPath));

    // Invalid time.
    order = makeOrder();
    order.time = "25:00:00";
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails(), orderPath,
                                              detailPath));

    CHECK_EQ(readFile(orderPath), std::string(""));
    CHECK_EQ(readFile(detailPath), std::string(""));

    removeTestFiles();
}

TEST_CASE(saved_order_and_details_round_trip)
{
    removeTestFiles();

    const Order order = makeOrder();
    std::vector<OrderDetail> details = makeDetails();

    OrderDetail second;
    second.orderId = "00011118";
    second.productId = 31;
    second.productBarcode = "579158";
    second.quantity = 2.25;
    details.push_back(second);

    CHECK(OrderManager::saveOrderTransaction(order, details, orderPath,
                                             detailPath));

    const std::vector<Order> orders = OrderManager::loadOrders(orderPath);
    CHECK_EQ(orders.size(), static_cast<std::size_t>(1));
    CHECK_EQ(orders[0].orderId, std::string("00011118"));
    CHECK_EQ(orders[0].date, std::string("2026-11-18"));
    CHECK_EQ(orders[0].time, std::string("19:30:00"));
    CHECK_EQ(orders[0].customerName, std::string("Ali Khan"));
    CHECK_EQ(orders[0].contact, std::string("0300 1234567"));
    CHECK_EQ(orders[0].status, std::string("paid"));
    CHECK(orders[0].paymentAmount == 1250.5);
    CHECK_EQ(orders[0].paymentMode, std::string("card"));
    CHECK_EQ(orders[0].merchant, std::string("Jazz Cash"));

    const std::vector<OrderDetail> loaded =
        OrderManager::loadOrderDetails("00011118", detailPath);
    CHECK_EQ(loaded.size(), static_cast<std::size_t>(2));
    CHECK_EQ(loaded[0].orderId, std::string("00011118"));
    CHECK_EQ(loaded[0].productId, 27);
    CHECK_EQ(loaded[0].productBarcode, std::string("428647"));
    CHECK(loaded[0].quantity == 4.5);
    CHECK_EQ(loaded[1].productId, 31);
    CHECK_EQ(loaded[1].productBarcode, std::string("579158"));
    CHECK(loaded[1].quantity == 2.25);

    Order found;
    CHECK(OrderManager::findOrder("00011118", found, orderPath));
    CHECK_EQ(found.customerName, std::string("Ali Khan"));
    CHECK(!OrderManager::findOrder("99999999", found, orderPath));

    removeTestFiles();
}

TEST_CASE(newline_in_customer_name_does_not_corrupt_the_record)
{
    removeTestFiles();
    Order order = makeOrder();
    order.customerName = "Evil\nInjected";

    CHECK(OrderManager::saveOrderTransaction(order, makeDetails(), orderPath,
                                             detailPath));

    const std::vector<Order> orders = OrderManager::loadOrders(orderPath);
    CHECK_EQ(orders.size(), static_cast<std::size_t>(1));
    CHECK_EQ(orders[0].customerName, std::string("Evil Injected"));

    removeTestFiles();
}

TEST_CASE(duplicate_order_id_is_rejected)
{
    removeTestFiles();
    const Order order = makeOrder();

    CHECK(OrderManager::saveOrderTransaction(order, makeDetails(), orderPath,
                                             detailPath));
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails(), orderPath,
                                              detailPath));
    CHECK_EQ(OrderManager::loadOrders(orderPath).size(),
             static_cast<std::size_t>(1));

    removeTestFiles();
}

TEST_CASE(generate_order_id_returns_empty_when_sequence_is_full)
{
    removeTestFiles();
    {
        std::ofstream file(orderPath);
        file << "99991118 2026-11-18 10:00:00 C - - in-progress 0 cash - #\n";
    }

    CHECK_EQ(OrderManager::generateOrderId("2026-11-18", orderPath),
             std::string(""));

    removeTestFiles();
}

TEST_CASE(malformed_order_lines_are_skipped_on_load)
{
    removeTestFiles();
    {
        std::ofstream file(orderPath);
        file << "garbage\n";
        file << "00011118 2026-11-18 19:30:00 Ali_Khan 0300_1234567 paid "
                "1250.5 card Jazz_Cash #\n";
        file << "00021118 2026-13-45 10:00:00 C - - paid 0 cash - #\n";
        file << "00031118 2026-11-18 10:00:00 C - - unknown 0 cash - #\n";
    }

    CHECK_EQ(OrderManager::loadOrders(orderPath).size(),
             static_cast<std::size_t>(1));

    removeTestFiles();
}

TEST_CASE(malformed_detail_lines_are_skipped_on_load)
{
    removeTestFiles();
    {
        std::ofstream file(detailPath);
        file << "00011118 27 428647 4.5 #\n";
        file << "00011118 0 428647 4.5 #\n";
        file << "00011118 27 012345 4.5 #\n";
        file << "00011118 27 428647 0 #\n";
        file << "00011118 27 428647 4.5\n";
        file << "00021118 31 579158 2 #\n";
    }

    const std::vector<OrderDetail> details =
        OrderManager::loadOrderDetails("00011118", detailPath);
    CHECK_EQ(details.size(), static_cast<std::size_t>(1));
    CHECK_EQ(details[0].productBarcode, std::string("428647"));
    CHECK(details[0].quantity == 4.5);

    removeTestFiles();
}
