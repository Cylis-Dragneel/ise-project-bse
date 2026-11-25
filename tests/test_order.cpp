// Test Suite for the Order subsystem: OrderManager, OrderPolicy, OrderService.
#include "test_framework.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../src/managers/OrderManager.h"
#include "../src/models/orderPolicy.h"
#include "../src/managers/orderService.h"
#include "../src/managers/catalogStockAdjuster.h"
#include "../src/core/catalog/FileProductRepository.h"
#include "../src/core/catalog/ProductCatalog.h"

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

// Fixtures are committed next to the sources so CI parses real files rather
// than only strings built in the test itself.
std::string fixturePath(const char *name)
{
    return std::string(QR_TEST_SOURCE_DIR) + "/tests/fixtures/" + name;
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
        std::error_code ec;
        fs::permissions(dir, fs::perms::owner_all, fs::perm_options::add, ec);
        fs::remove_all(dir, ec);
        fs::create_directories(dir);
        paths.order = (dir / "Order.txt").string();
        paths.detail = (dir / "OrderDetail.txt").string();
        std::ofstream(paths.order).close();
        std::ofstream(paths.detail).close();
    }
    ~Env() {
        std::error_code ec;
        fs::permissions(dir, fs::perms::owner_all, fs::perm_options::add, ec);
        fs::remove_all(dir, ec);
    }
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

// 1. OrderManager & Basic Persistence Tests
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

TEST_CASE(committed_order_fixtures_parse_and_round_trip)
{
    removeTestFiles();

    const std::string orderFixture = fixturePath("Order.txt");
    const std::string detailFixture = fixturePath("OrderDetail.txt");

    const std::vector<Order> orders = OrderManager::loadOrders(orderFixture);
    CHECK_EQ(orders.size(), static_cast<std::size_t>(3));

    CHECK_EQ(orders[0].orderId, std::string("00011118"));
    CHECK_EQ(orders[0].customerName, std::string("Ali Khan"));
    CHECK_EQ(orders[0].status, std::string("paid"));
    CHECK(orders[0].paymentAmount == 1250.5);

    CHECK_EQ(orders[1].orderId, std::string("00021118"));
    CHECK_EQ(orders[1].status, std::string("in-progress"));
    CHECK_EQ(orders[1].customerName, std::string(""));

    CHECK_EQ(orders[2].orderId, std::string("00011119"));
    CHECK_EQ(orders[2].paymentMode, std::string("wallet"));

    // Every committed row must survive a load/save round trip unchanged. A
    // writer/reader format drift then fails CI instead of only the app.
    for (const Order &order : orders)
    {
        const std::vector<OrderDetail> details =
            OrderManager::loadOrderDetails(order.orderId, detailFixture);
        CHECK(!details.empty());
        CHECK(OrderManager::saveOrderTransaction(order, details, orderPath,
                                                 detailPath));
    }

    CHECK_EQ(readFile(orderPath), readFile(orderFixture.c_str()));
    CHECK_EQ(readFile(detailPath), readFile(detailFixture.c_str()));

    removeTestFiles();
}


// 2. updateOrderTransaction Tests
TEST_CASE(update_unknown_order_id_leaves_both_files_unchanged)
{
    removeTestFiles();

    // Two real orders so "unchanged" means something.
    Order first = makeOrder();
    CHECK(OrderManager::saveOrderTransaction(first, makeDetails(), orderPath,
                                             detailPath));

    Order second = makeOrder();
    second.orderId = "00021118";
    second.status = "on-hold";
    std::vector<OrderDetail> secondDetails =
        std::vector<OrderDetail>(1, det("00021118", 31, "579158", 2));
    CHECK(OrderManager::saveOrderTransaction(second, secondDetails, orderPath,
                                             detailPath));

    const std::string orderBefore = readFile(orderPath);
    const std::string detailBefore = readFile(detailPath);

    // A perfectly valid order whose id is simply not in the file.
    Order ghost = makeOrder();
    ghost.orderId = "00091118";
    CHECK(!OrderManager::updateOrderTransaction(
        ghost, std::vector<OrderDetail>(1, det("00091118", 27, "428647", 1)),
        orderPath, detailPath));

    CHECK_EQ(readFile(orderPath), orderBefore);
    CHECK_EQ(readFile(detailPath), detailBefore);
    CHECK(!fs::exists(std::string(orderPath) + ".tmp"));
    CHECK(!fs::exists(std::string(detailPath) + ".tmp"));

    removeTestFiles();
}

// The header (Order.txt) commit is the second rename. When it fails after the
// detail file was already replaced, the detail file must be put back so the two
// files still agree.
//
// The failure is produced without changing production code: Order.txt lives in
// a read-only directory that already holds a writable Order.txt.tmp, so staging
// the temp file succeeds but the rename into the directory is refused. Where
// permissions are not enforced (running as root, Windows) the setup cannot
// force the failure, so the test reports a skip instead of passing falsely.
TEST_CASE(update_restores_detail_file_when_order_commit_fails)
{
    Env e("commitfail");

    const fs::path orderDir = e.dir / "orders";
    fs::create_directories(orderDir);
    const std::string orderFile = (orderDir / "Order.txt").string();
    const std::string detailFile = (e.dir / "OrderDetail.txt").string();

    Order order = makeOrder();
    CHECK(OrderManager::saveOrderTransaction(order, makeDetails(), orderFile,
                                             detailFile));
    const std::string orderBefore = readFile(orderFile.c_str());
    const std::string detailBefore = readFile(detailFile.c_str());
    CHECK(!orderBefore.empty() && !detailBefore.empty());

    std::ofstream((orderDir / "Order.txt.tmp").string()).close();

    std::error_code ec;
    fs::permissions(orderDir,
                    fs::perms::owner_read | fs::perms::owner_exec,
                    fs::perm_options::replace, ec);

    // Probe: can we still create files in the directory?
    const fs::path probe = orderDir / "probe";
    std::ofstream(probe.string()).close();
    if (fs::exists(probe))
    {
        fs::remove(probe, ec);
        fs::permissions(orderDir, fs::perms::owner_all,
                        fs::perm_options::replace, ec);
        std::cerr << "SKIPPED update_restores_detail_file_when_order_commit_"
                     "fails: directory permissions are not enforced here "
                     "(root or Windows)\n";
        return;
    }

    // Changes both the header row and the detail rows, so a missing restore
    // would leave a visibly different OrderDetail file behind.
    Order changed = order;
    changed.customerName = "Changed Name";
    changed.paymentAmount = 99;
    const bool updated = OrderManager::updateOrderTransaction(
        changed, std::vector<OrderDetail>(1, det("00011118", 31, "579158", 9)),
        orderFile, detailFile);

    fs::permissions(orderDir, fs::perms::owner_all, fs::perm_options::replace,
                    ec);

    CHECK(!updated);
    CHECK_EQ(readFile(orderFile.c_str()), orderBefore);
    CHECK_EQ(readFile(detailFile.c_str()), detailBefore);
    CHECK(!fs::exists(detailFile + ".tmp"));
}

// 3. OrderPolicy Tests
TEST_CASE(policy_rules_master)
{
    BillStamp none;

    // 7 days old: inside the window, but a paid edit always needs approval.
    Order p = mk("00011116", "2026-11-16", "paid");
    CHECK(!OrderPolicy::canEdit(p, none, "2026-11-23"));
    CHECK(OrderPolicy::requiresManagerApproval(p, none, "2026-11-23"));
    CHECK(OrderPolicy::canEdit(p, none, "2026-11-23", true));

    // 8 days old: outside the window, not editable even with approval.
    Order q = mk("00011115", "2026-11-15", "paid");
    CHECK(!OrderPolicy::canEdit(q, none, "2026-11-23"));
    CHECK(!OrderPolicy::requiresManagerApproval(q, none, "2026-11-23"));
    CHECK(!OrderPolicy::canEdit(q, none, "2026-11-23", true));

    // The bill date is what counts: a recent bill brings the old order back
    // into the window, where approval is still required.
    BillStamp bill;
    bill.hasBill = true;
    bill.date = "2026-11-22";
    bill.time = "09:00:00";
    CHECK(!OrderPolicy::canEdit(q, bill, "2026-11-23"));
    CHECK(OrderPolicy::requiresManagerApproval(q, bill, "2026-11-23"));
    CHECK(OrderPolicy::canEdit(q, bill, "2026-11-23", true));

    CHECK(OrderPolicy::canEdit(mk("00011101", "2026-01-01", "on-hold"), none, "2026-11-23"));
    CHECK(OrderPolicy::canEdit(mk("00011101", "2026-01-01", "in-progress"), none, "2026-11-23"));
    CHECK(!OrderPolicy::canEdit(mk("00011101", "2026-11-22", "archived"), none, "2026-11-23", true));

    CHECK(OrderPolicy::canTransition("on-hold", "in-progress"));
    CHECK(OrderPolicy::canTransition("in-progress", "paid"));
    CHECK(!OrderPolicy::canTransition("paid", "on-hold"));
}


// 4. OrderService, Stock & Workflow Tests
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
    CHECK(OrderManager::loadOrdersByStatus("on-hold", e.paths.order).size() == 1);

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
    Order recent = mk("00011116", "2026-11-16", "paid");
    CHECK(OrderService::saveNewOrder(recent, {det("", 1, "428647", 3)}, st, e.paths) == OrderResult::Ok);
    BillStamp none;
    Order upd = recent; upd.customerName = "New Name";
    CHECK(OrderService::editOrder(upd, {det("", 1, "428647", 2)}, none, "2026-11-23", false, st, e.paths) == OrderResult::NeedsApproval);
    CHECK(OrderService::editOrder(upd, {det("", 1, "428647", 2)}, none, "2026-11-23", true, st, e.paths) == OrderResult::Ok);

    // Older than the 7-day window: not editable, approval or not.
    Env oldEnv("approval_old");
    FakeStock oldStock;
    oldStock.qty = {{1, 10}};
    Order old = mk("00011110", "2026-11-10", "paid");
    CHECK(OrderService::saveNewOrder(old, {det("", 1, "428647", 1)}, oldStock, oldEnv.paths) == OrderResult::Ok);
    Order oldUpd = old; oldUpd.customerName = "Older";
    CHECK(OrderService::editOrder(oldUpd, {det("", 1, "428647", 1)}, none, "2026-11-23", true, oldStock, oldEnv.paths) == OrderResult::NotEditable);

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

TEST_CASE(catalog_stock_adjuster_bridges_order_deltas)
{
    const std::string productPath = "test_order_products_tmp.txt";
    {
        std::ofstream out(productPath.c_str(), std::ios::trunc);
        out << readFile(fixturePath("Product.txt").c_str());
    }

    ProductCatalog catalog(std::make_shared<FileProductRepository>(productPath));
    catalog.load();
    CatalogStockAdjuster adjuster(catalog);

    const double before = catalog.findByBarcode("428647")->availableQuantity();
    CHECK(adjuster.adjustStock({{1, "428647", -3.0}}));
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == before - 3.0);

    // Unknown product and below-zero movements return false, nothing changes.
    CHECK(!adjuster.adjustStock({{999, "999999", -1.0}}));
    CHECK(!adjuster.adjustStock({{1, "428647", -1e9}}));
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == before - 3.0);

    // productId is used when the delta carries no barcode.
    CHECK(adjuster.adjustStock({{1, "", 2.0}}));
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == before - 1.0);

    std::remove(productPath.c_str());
}