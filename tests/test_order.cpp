// Master Test Suite for Order Subsystem: OrderManager, OrderPolicy, and OrderService.
#include "test_framework.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../src/managers/OrderManager.h"
#include "../src/models/orderPolicy.h"
#include "../src/managers/orderService.h"

namespace fs = std::filesystem;

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
    order.orderId = "00011118";
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

OrderDetail det(const std::string &id, int pid, const std::string &bc, double q) {
    OrderDetail d;
    d.orderId = id; d.productId = pid; d.productBarcode = bc; d.quantity = q;
    return d;
}

Order mk(const std::string &id, const std::string &date, const std::string &status) {
    Order o;
    o.orderId = id; o.date = date; o.time = "10:00:00"; o.status = status;
    o.customerName = "Ali Khan";
    return o;
}

struct Env {
    fs::path dir;
    OrderPaths paths;
    Env(const std::string &name) {
        dir = fs::temp_directory_path() / ("orderMaster_" + name);
        fs::remove_all(dir);
        fs::create_directories(dir);
        paths.order = (dir / "Order.txt").string();
        paths.detail = (dir / "OrderDetail.txt").string();
        std::ofstream(paths.order).close();
        std::ofstream(paths.detail).close();
    }
    ~Env() { fs::remove_all(dir); }
};

struct FakeStock : IStockAdjuster {
    std::map<int, double> qty;
    int calls = 0;
    bool adjustStock(const std::vector<StockDelta> &deltas) override {
        ++calls;
        std::map<int, double> next = qty;
        for (const StockDelta &d : deltas) {
            auto it = next.find(d.productId);
            if (it == next.end()) return false;
            it->second += d.delta;
            if (it->second < -1e-9) return false;
        }
        qty = next;
        return true;
    }
};

} // namespace

// ==========================================
// 1. OrderManager & Basic Persistence Tests
// ==========================================
TEST_CASE(order_id_generation_master)
{
    removeTestFiles();
    {
        std::ofstream file(orderPath);
        file << "00011117 2026-11-17 10:00:00 C 1118 - in-progress 0 cash - #\n";
        file << "00031118 2026-11-18 10:00:00 C - - in-progress 0 cash - #\n";
    }

    CHECK_EQ(OrderManager::generateOrderId("2026-11-18", orderPath), std::string("00041118"));
    CHECK_EQ(OrderManager::generateOrderId("2026-11-19", orderPath), std::string("00011119"));
    CHECK_EQ(OrderManager::generateOrderId("bad-date", orderPath), std::string(""));

    removeTestFiles();
}

TEST_CASE(transaction_writing_master)
{
    removeTestFiles();
    const Order order = makeOrder();

    CHECK(OrderManager::saveOrderTransaction(order, makeDetails(), orderPath, detailPath));

    CHECK_EQ(readFile(orderPath),
             std::string("00011118 2026-11-18 19:30:00 Ali_Khan 0300_1234567 paid 1250.5 card Jazz_Cash #\n"));
    CHECK_EQ(readFile(detailPath), std::string("00011118 27 428647 4.5 #\n"));

    removeTestFiles();
}

TEST_CASE(optional_fields_master)
{
    removeTestFiles();
    Order order = makeOrder();
    order.contact.clear();
    order.merchant.clear();

    CHECK(OrderManager::saveOrderTransaction(order, makeDetails(), orderPath, detailPath));
    CHECK_EQ(readFile(orderPath),
             std::string("00011118 2026-11-18 19:30:00 Ali_Khan - paid 1250.5 card - #\n"));

    removeTestFiles();
}

TEST_CASE(invalid_records_rejected_master)
{
    removeTestFiles();
    Order order = makeOrder();

    order.status = "Incomplete";
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails(), orderPath, detailPath));

    order = makeOrder();
    order.paymentMode = "Crypto";
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails(), orderPath, detailPath));

    order = makeOrder();
    order.paymentAmount = -5.0;
    CHECK(!OrderManager::saveOrderTransaction(order, makeDetails(), orderPath, detailPath));

    removeTestFiles();
}

// ==========================================
// 2. OrderPolicy Tests
// ==========================================
TEST_CASE(policy_rules_master)
{
    BillStamp none;
    Order p = mk("00011116", "2026-11-16", "paid");
    CHECK(OrderPolicy::canEdit(p, none, "2026-11-23"));
    CHECK(!OrderPolicy::requiresManagerApproval(p, none, "2026-11-23"));

    Order q = mk("00011115", "2026-11-15", "paid");
    CHECK(!OrderPolicy::canEdit(q, none, "2026-11-23"));
    CHECK(OrderPolicy::requiresManagerApproval(q, none, "2026-11-23"));
    CHECK(OrderPolicy::canEdit(q, none, "2026-11-23", true));

    BillStamp bill;
    bill.hasBill = true;
    bill.date = "2026-11-22";
    bill.time = "09:00:00";
    CHECK(OrderPolicy::canEdit(q, bill, "2026-11-23"));

    CHECK(OrderPolicy::canEdit(mk("00011101", "2026-01-01", "on-hold"), none, "2026-11-23"));
    CHECK(OrderPolicy::canEdit(mk("00011101", "2026-01-01", "in-progress"), none, "2026-11-23"));
    CHECK(!OrderPolicy::canEdit(mk("00011101", "2026-11-22", "archived"), none, "2026-11-23", true));

    CHECK(OrderPolicy::canTransition("on-hold", "in-progress"));
    CHECK(OrderPolicy::canTransition("in-progress", "paid"));
    CHECK(!OrderPolicy::canTransition("paid", "on-hold"));
}

// ==========================================
// 3. OrderService, Stock & Workflow Tests
// ==========================================
TEST_CASE(stock_deltas_master) {
    auto r = OrderService::reserveDeltas({det("x", 1, "428647", 2), det("x", 1, "428647", 1.5), det("x", 2, "571234", 1)});
    CHECK(r.size() == 2 && r[0].productId == 1 && r[0].delta == -3.5 && r[1].delta == -1);

    auto e = OrderService::editDeltas({det("x", 1, "428647", 2), det("x", 2, "571234", 3)},
                                      {det("x", 1, "428647", 5), det("x", 3, "381111", 1)});
    CHECK(e.size() == 3);
    for (auto &d : e) {
        if (d.productId == 1) CHECK(d.delta == -3);
        if (d.productId == 2) CHECK(d.delta == 3);
        if (d.productId == 3) CHECK(d.delta == -1);
    }
}

TEST_CASE(update_in_place_master) {
    Env e("update");
    FakeStock st;
    st.qty = {{1, 100}, {2, 100}, {3, 100}};
    Order a = mk("00011120", "2026-11-20", "paid");
    Order b = mk("00021120", "2026-11-20", "on-hold");
    Order c = mk("00031120", "2026-11-20", "paid");
    CHECK(OrderService::saveNewOrder(a, {det("", 1, "428647", 2)}, st, e.paths) == OrderResult::Ok);
    CHECK(OrderService::saveNewOrder(b, {det("", 1, "428647", 1), det("", 2, "571234", 3)}, st, e.paths) == OrderResult::Ok);
    CHECK(OrderService::saveNewOrder(c, {det("", 3, "381111", 5)}, st, e.paths) == OrderResult::Ok);

    Order b2 = b;
    b2.status = "in-progress";
    b2.customerName = "Sara";
    CHECK(OrderManager::updateOrderTransaction(
        b2, {det("00021120", 2, "571234", 4)}, e.paths.order, e.paths.detail));

    std::vector<Order> all = OrderManager::loadOrders(e.paths.order);
    CHECK(all.size() == 3);
    CHECK(all[1].status == "in-progress" && all[1].customerName == "Sara");
}

TEST_CASE(flow_and_stock_master) {
    Env e("flow");
    FakeStock st;
    st.qty = {{1, 10}, {2, 10}};
    Order o;
    o.date = "2026-11-23";
    CHECK(OrderService::holdOrder(o, {det("", 1, "428647", 4)}, st, e.paths) == OrderResult::Ok);
    CHECK(o.orderId == "00011123" && o.status == "on-hold");
    CHECK(st.qty[1] == 6);
    CHECK(OrderManager::loadOrders("on-hold", e.paths.order).size() == 1);

    Order r; std::vector<OrderDetail> rd;
    CHECK(OrderService::resumeOrder("00011123", r, rd, e.paths) == OrderResult::Ok);
    CHECK(rd.size() == 1 && rd[0].quantity == 4);

    BillStamp none;
    r.status = "in-progress";
    CHECK(OrderService::editOrder(r, {det("", 1, "428647", 7), det("", 2, "571234", 2)},
                                  none, "2026-11-23", false, st, e.paths) == OrderResult::Ok);
    CHECK(st.qty[1] == 3 && st.qty[2] == 8);
}

TEST_CASE(approval_and_archive_unpaid_master) {
    Env e("approval");
    FakeStock st;
    st.qty = {{1, 10}};
    Order old = mk("00011110", "2026-11-10", "paid");
    CHECK(OrderService::saveNewOrder(old, {det("", 1, "428647", 3)}, st, e.paths) == OrderResult::Ok);
    BillStamp none;
    Order upd = old; upd.customerName = "New Name";
    CHECK(OrderService::editOrder(upd, {det("", 1, "428647", 2)}, none, "2026-11-23", false, st, e.paths) == OrderResult::NeedsApproval);
    CHECK(OrderService::editOrder(upd, {det("", 1, "428647", 2)}, none, "2026-11-23", true, st, e.paths) == OrderResult::Ok);

    Order h; h.date = "2026-11-23";
    CHECK(OrderService::holdOrder(h, {det("", 1, "428647", 5)}, st, e.paths) == OrderResult::Ok);
    CHECK(OrderService::archiveOrder(h.orderId, st, e.paths) == OrderResult::Ok);
    CHECK(st.qty[1] == 8); // Unpaid archive returned stock
}

TEST_CASE(write_failure_rolls_back_stock_master) {
    Env e("rollback");
    FakeStock st;
    st.qty = {{1, 10}};
    OrderPaths bad = e.paths;
    bad.detail = (e.dir / "nope" / "OrderDetail.txt").string();
    Order o; o.date = "2026-11-23";
    CHECK(OrderService::saveNewOrder(o, {det("", 1, "428647", 4)}, st, bad) == OrderResult::WriteFailed);
    CHECK(st.qty[1] == 10);
}