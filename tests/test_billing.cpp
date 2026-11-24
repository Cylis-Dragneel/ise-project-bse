#include "test_framework.h"

#include "billing/BillCalculator.h"
#include "billing/BillRecordCodec.h"
#include "billing/BillingService.h"
#include "billing/FileBillRepository.h"
#include "catalog/FileProductRepository.h"
#include "catalog/ProductCatalog.h"
#include "Order.h"
#include "OrderManager.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const char *billPath = "test_inc3_bill_tmp.txt";
const char *detailPath = "test_inc3_detail_tmp.txt";
const char *orderPath = "test_inc3_order_tmp.txt";
const char *orderDetailPath = "test_inc3_order_detail_tmp.txt";
const char *productPath = "test_inc3_product_tmp.txt";

std::string fixturePath(const char *name)
{
    return std::string(QR_TEST_SOURCE_DIR) + "/tests/fixtures/" + name;
}

std::string readFile(const std::string &path)
{
    std::ifstream file(path.c_str());
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

void writeFile(const std::string &path, const std::string &content)
{
    std::ofstream file(path.c_str(), std::ios::trunc);
    file << content;
}

void removeAllTempFiles()
{
    std::remove(billPath);
    std::remove(detailPath);
    std::remove(orderPath);
    std::remove(orderDetailPath);
    std::remove(productPath);
}

struct CatalogFixture
{
    CatalogFixture()
        : catalog(std::make_shared<FileProductRepository>(fixturePath("Product.txt")))
    {
        catalog.load();
    }

    ProductCatalog catalog;
};

Order makeOrder(const std::string &orderId = "00011124",
                const std::string &date = "2026-11-24",
                const std::string &status = "paid")
{
    Order order;
    order.orderId = orderId;
    order.date = date;
    order.time = "11:45:00";
    order.customerName = "Ali Khan";
    order.contact = "0300 1234567";
    order.status = status;
    order.paymentAmount = 820.0;
    order.paymentMode = "card";
    order.merchant = "Jazz Cash";
    return order;
}

OrderDetail makeDetail(const std::string &orderId, int productId,
                       const std::string &barcode, double quantity)
{
    OrderDetail detail;
    detail.orderId = orderId;
    detail.productId = productId;
    detail.productBarcode = barcode;
    detail.quantity = quantity;
    return detail;
}

void checkClose(double actual, double expected)
{
    if (std::fabs(actual - expected) > 1e-9)
    {
        std::ostringstream message;
        message << "expected " << actual << " to be close to " << expected;
        throw testfw::AssertionFailure(message.str());
    }
}

class FixedSeasonProvider : public ISeasonProvider
{
private:
    double percent_;

public:
    explicit FixedSeasonProvider(double percent) : percent_(percent) {}
    double activePercent(const std::string &) const override { return percent_; }
};

class FailingBillRepository : public IBillRepository
{
public:
    std::vector<Bill> loadAll() override { return {}; }
    void saveAll(const std::vector<Bill> &) override
    {
        throw std::runtime_error("simulated Bill.txt write failure");
    }
    void append(const Bill &) override
    {
        throw std::runtime_error("simulated Bill.txt append failure");
    }
};

} // namespace

TEST_CASE(bill_calculator_rounds_money_to_two_decimals)
{
    checkClose(BillCalculator::roundMoney(1.234), 1.23);
    checkClose(BillCalculator::roundMoney(1.236), 1.24);
    checkClose(BillCalculator::roundMoney(0.005), 0.01);
    checkClose(BillCalculator::roundMoney(0.0), 0.0);
    checkClose(BillCalculator::roundMoney(1500.0), 1500.0);
    CHECK_EQ(BillCalculator::roundMoney(-0.001), 0.0);
    CHECK_THROWS_AS(BillCalculator::roundMoney(std::nan("")), std::invalid_argument);
}

TEST_CASE(bill_id_starts_at_one_and_increments)
{
    CHECK_EQ(BillCalculator::nextBillId({}), std::string("000000001"));

    Bill b1;
    b1.billId = "000000001";
    Bill b2;
    b2.billId = "000000002";
    CHECK_EQ(BillCalculator::nextBillId({b1, b2}), std::string("000000003"));

    Bill bMax;
    bMax.billId = "999999999";
    CHECK_THROWS_AS(BillCalculator::nextBillId({bMax}), std::overflow_error);

    Bill bBad;
    bBad.billId = "42";
    CHECK_THROWS_AS(BillCalculator::nextBillId({bBad}), std::invalid_argument);
}

TEST_CASE(bill_header_record_round_trips)
{
    Bill header;
    header.billId = "000000001";
    header.customerName = "Ali Khan";
    header.orderId = "00011124";
    header.totalNoTax = 800.0;
    header.totalTax = 40.0;
    header.totalDiscount = 20.0;
    header.totalPayable = 820.0;
    header.amountReceived = 1000.0;
    header.balanceReturned = 180.0;
    header.paymentStatus = "paid";
    header.seasonalDiscount = 0.0;
    header.specialDiscountPercent = 0.0;
    header.billDate = "2026-11-24";
    header.billTime = "11:45:00";

    const std::string encoded = BillRecordCodec::format(header);
    CHECK(!encoded.empty());
    CHECK_EQ(encoded.back(), '#');

    const Bill parsed = BillRecordCodec::parse(encoded);
    CHECK_EQ(parsed.billId, std::string("000000001"));
    CHECK_EQ(parsed.customerName, std::string("Ali_Khan"));
    CHECK_EQ(parsed.orderId, std::string("00011124"));
    checkClose(parsed.totalNoTax, 800.0);
    checkClose(parsed.totalTax, 40.0);
    checkClose(parsed.totalDiscount, 20.0);
    checkClose(parsed.totalPayable, 820.0);
    checkClose(parsed.amountReceived, 1000.0);
    checkClose(parsed.balanceReturned, 180.0);
    CHECK_EQ(parsed.paymentStatus, std::string("paid"));
    checkClose(parsed.seasonalDiscount, 0.0);
    checkClose(parsed.specialDiscountPercent, 0.0);
    CHECK_EQ(parsed.billDate, std::string("2026-11-24"));
    CHECK_EQ(parsed.billTime, std::string("11:45:00"));

    header.customerName = "   ";
    CHECK_EQ(BillRecordCodec::parse(BillRecordCodec::format(header)).customerName,
             std::string("-"));
}

TEST_CASE(bill_header_record_rejects_malformed_records)
{
    const std::string good = BillRecordCodec::format([]() {
        Bill b;
        b.billId = "000000001";
        b.customerName = "Ali";
        b.orderId = "00011124";
        b.totalPayable = 100.0;
        b.amountReceived = 100.0;
        b.paymentStatus = "paid";
        b.billDate = "2026-11-24";
        b.billTime = "11:45:00";
        return b;
    }());

    CHECK_THROWS_AS(BillRecordCodec::parse("000000001 Ali 00011124 100 0 0 100 paid #"),
                    std::invalid_argument);
    CHECK_THROWS_AS(
        BillRecordCodec::parse("00000001 Ali 00011124 100 0 0 100 100 0 paid 0 0 2026-11-24 11:45:00#"),
        std::invalid_argument);
    CHECK_THROWS_AS(
        BillRecordCodec::parse("000000001 Ali 1124 100 0 0 100 100 0 paid 0 0 2026-11-24 11:45:00#"),
        std::invalid_argument);
    CHECK_THROWS_AS(
        BillRecordCodec::parse("000000001 Ali 00011124 100 0 0 100 100 0 refunded 0 0 2026-11-24 11:45:00#"),
        std::invalid_argument);
    CHECK_THROWS_AS(
        BillRecordCodec::parse("000000001 Ali 00011124 100 0 0 100 100 0 paid 0 9 2026-11-24 11:45:00#"),
        std::invalid_argument);
    CHECK_THROWS_AS(BillRecordCodec::splitRecords(good.substr(0, good.size() - 1)),
                    std::invalid_argument);
}

TEST_CASE(bill_detail_record_round_trips)
{
    BillLine line;
    line.billId = "000000001";
    line.productAcronym = "MILK1L";
    line.unitPrice = 280.0;
    line.quantity = 2.0;
    line.discountTotal = 20.0;

    const std::string encoded = BillDetailRecordCodec::format(line);
    CHECK(!encoded.empty());
    CHECK_EQ(encoded.back(), '#');

    const BillLine parsed = BillDetailRecordCodec::parse(encoded);
    CHECK_EQ(parsed.billId, std::string("000000001"));
    CHECK_EQ(parsed.productAcronym, std::string("MILK1L"));
    checkClose(parsed.unitPrice, 280.0);
    checkClose(parsed.quantity, 2.0);
    checkClose(parsed.discountTotal, 20.0);

    const BillLine bare = BillDetailRecordCodec::parse(encoded.substr(0, encoded.size() - 1));
    CHECK_EQ(bare.productAcronym, std::string("MILK1L"));
}

TEST_CASE(bill_detail_record_rejects_malformed_records)
{
    const std::string good = BillDetailRecordCodec::format([]() {
        BillLine l;
        l.billId = "000000001";
        l.productAcronym = "MILK1L";
        l.unitPrice = 280.0;
        l.quantity = 2.0;
        l.discountTotal = 20.0;
        return l;
    }());

    CHECK_THROWS_AS(BillDetailRecordCodec::parse("000000001 MILK1L 280 2#"),
                    std::invalid_argument);
    CHECK_THROWS_AS(BillDetailRecordCodec::parse("000001 MILK1L 280 2 20#"),
                    std::invalid_argument);
    CHECK_THROWS_AS(BillDetailRecordCodec::parse("000000001 TOOLONGACRONYM 280 2 20#"),
                    std::invalid_argument);
    CHECK_THROWS_AS(BillDetailRecordCodec::parse("000000001 MILK1L 280 0 0#"),
                    std::invalid_argument);
    CHECK_THROWS_AS(BillDetailRecordCodec::parse("000000001 MILK1L 280 2 -5#"),
                    std::invalid_argument);
    CHECK_THROWS_AS(BillDetailRecordCodec::splitRecords(good.substr(0, good.size() - 1)),
                    std::invalid_argument);
}

TEST_CASE(billing_service_quotes_without_writing_and_without_enforcing_payment)
{
    removeAllTempFiles();

    CatalogFixture fixture;
    auto billRepo = std::make_shared<FileBillRepository>(billPath);
    auto detailRepo = std::make_shared<FileBillDetailRepository>(detailPath);
    BillingService service(fixture.catalog, billRepo, detailRepo);

    const Order order = makeOrder("00011124", "2026-11-24", "paid");
    std::vector<OrderDetail> cart = {makeDetail("00011124", 1, "428647", 1.0)};

    // Quote with amountReceived = 0 should succeed for preview even if order is "paid"
    const Bill preview = service.quote(order, cart, 0.0, 0.0, "2026-11-24", "12:00:00");
    CHECK(preview.billId.empty());
    checkClose(preview.totalPayable, 284.0);
    CHECK(billRepo->loadAll().empty());
    CHECK(detailRepo->loadAll().empty());

    removeAllTempFiles();
}

TEST_CASE(billing_service_seasonal_and_special_discounts_and_rounding)
{
    removeAllTempFiles();

    CatalogFixture fixture;
    auto billRepo = std::make_shared<FileBillRepository>(billPath);
    auto detailRepo = std::make_shared<FileBillDetailRepository>(detailPath);
    auto season10 = std::make_shared<FixedSeasonProvider>(10.0);

    BillingService service(fixture.catalog, billRepo, detailRepo, season10);

    // Milk (280, tax 14, disc 10) x 2 + Sugar (160, tax 8, disc 0) x 1.5
    // totalNoTax = 800, totalTax = 40, totalDiscount = 20 -> base = 820
    // seasonal (10%) = 82.00 -> afterSeasonal = 738.00
    // special (5%) = 36.90 -> totalPayable = 701.10
    std::vector<OrderDetail> cart = {
        makeDetail("00011124", 1, "428647", 2.0),
        makeDetail("00011124", 3, "579158", 1.5)
    };

    const Order order = makeOrder("00011124", "2026-11-24", "paid");
    const Bill bill = service.generateBill(order, cart, 5.0, 1000.0, "2026-11-24", "12:00:00");

    CHECK_EQ(bill.billId, std::string("000000001"));
    CHECK_EQ(bill.customerName, std::string("Ali_Khan"));
    checkClose(bill.totalNoTax, 800.0);
    checkClose(bill.totalTax, 40.0);
    checkClose(bill.totalDiscount, 20.0);
    checkClose(bill.seasonalDiscount, 82.0);
    checkClose(bill.specialDiscountPercent, 5.0);
    checkClose(bill.totalPayable, 701.10);
    checkClose(bill.amountReceived, 1000.0);
    checkClose(bill.balanceReturned, 298.90);

    // Special discount > 8% rejected
    CHECK_THROWS_AS(service.generateBill(order, cart, 8.5, 1000.0, "2026-11-24", "12:05:00"),
                    std::invalid_argument);

    // Amount received < totalPayable on paid bill rejected
    CHECK_THROWS_AS(service.generateBill(order, cart, 0.0, 500.0, "2026-11-24", "12:10:00"),
                    std::invalid_argument);

    removeAllTempFiles();
}

TEST_CASE(billing_service_rejects_duplicate_bill_ids)
{
    removeAllTempFiles();

    CatalogFixture fixture;
    auto billRepo = std::make_shared<FileBillRepository>(billPath);
    auto detailRepo = std::make_shared<FileBillDetailRepository>(detailPath);
    BillingService service(fixture.catalog, billRepo, detailRepo);

    const Order order = makeOrder("00011124", "2026-11-24", "paid");
    std::vector<OrderDetail> cart = {makeDetail("00011124", 1, "428647", 1.0)};

    const Bill first = service.generateBill(order, cart, 0.0, 500.0, "2026-11-24", "13:00:00");
    CHECK_THROWS_AS(billRepo->append(first), std::invalid_argument);

    writeFile(billPath,
              BillRecordCodec::format(first) + "\n" +
              BillRecordCodec::format(first) + "\n");
    CHECK_THROWS_AS(billRepo->loadAll(), std::invalid_argument);

    removeAllTempFiles();
}

TEST_CASE(billing_service_rolls_back_details_when_header_write_fails)
{
    removeAllTempFiles();

    CatalogFixture fixture;
    auto realBillRepo = std::make_shared<FileBillRepository>(billPath);
    auto detailRepo = std::make_shared<FileBillDetailRepository>(detailPath);
    BillingService okService(fixture.catalog, realBillRepo, detailRepo);

    const Order order = makeOrder("00011124", "2026-11-24", "paid");
    std::vector<OrderDetail> cart = {makeDetail("00011124", 1, "428647", 1.0)};
    okService.generateBill(order, cart, 0.0, 500.0, "2026-11-24", "13:00:00");
    CHECK_EQ(detailRepo->loadAll().size(), static_cast<std::size_t>(1));

    auto failingBillRepo = std::make_shared<FailingBillRepository>();
    BillingService failingService(fixture.catalog, failingBillRepo, detailRepo);
    CHECK_THROWS_AS(failingService.generateBill(order, cart, 0.0, 500.0, "2026-11-24", "13:05:00"),
                    std::runtime_error);
    CHECK_EQ(detailRepo->loadAll().size(), static_cast<std::size_t>(1));

    removeAllTempFiles();
}

TEST_CASE(billing_service_replaces_a_bill_in_place_and_rejects_orphan_details)
{
    removeAllTempFiles();

    CatalogFixture fixture;
    auto billRepo = std::make_shared<FileBillRepository>(billPath);
    auto detailRepo = std::make_shared<FileBillDetailRepository>(detailPath);
    BillingService service(fixture.catalog, billRepo, detailRepo);

    const Order order = makeOrder("00011124", "2026-11-24", "paid");
    std::vector<OrderDetail> cart = {
        makeDetail("00011124", 1, "428647", 2.0),
        makeDetail("00011124", 3, "579158", 1.0)
    };

    const Bill original = service.generateBill(order, cart, 0.0, 1000.0, "2026-11-24", "14:00:00");
    CHECK_EQ(original.billId, std::string("000000001"));

    std::vector<OrderDetail> editedCart = {makeDetail("00011124", 1, "428647", 1.0)};
    const Bill updated =
        service.updateExistingBill("000000001", order, editedCart, 0.0, 500.0, "2026-11-24", "14:30:00");

    CHECK_EQ(updated.billId, std::string("000000001"));
    checkClose(updated.totalPayable, 284.0);
    checkClose(updated.balanceReturned, 216.0);
    CHECK_EQ(updated.billTime, std::string("14:30:00"));

    Bill found;
    CHECK(service.findBillByOrderId("00011124", found));
    CHECK_EQ(found.lines.size(), static_cast<std::size_t>(1));

    // Orphan detail row whose billId is not in Bill.txt must be rejected on loadBills()
    writeFile(detailPath,
              readFile(detailPath) + "999999999 MILK1L 280 1 10 #\n");
    CHECK_THROWS_AS(service.loadBills(), std::invalid_argument);

    removeAllTempFiles();
}

TEST_CASE(billing_end_to_end_with_order_detail_file)
{
    removeAllTempFiles();

    writeFile(productPath, readFile(fixturePath("Product.txt")));
    ProductCatalog catalog(std::make_shared<FileProductRepository>(productPath));
    catalog.load();

    const Order order = makeOrder("00011124", "2026-11-24", "paid");
    const std::vector<OrderDetail> details = {
        makeDetail("00011124", 1, "428647", 2.0),
        makeDetail("00011124", 3, "579158", 1.5)
    };

    CHECK(OrderManager::saveOrderTransaction(order, details, orderPath, orderDetailPath));

    auto billRepo = std::make_shared<FileBillRepository>(billPath);
    auto detailRepo = std::make_shared<FileBillDetailRepository>(detailPath);
    BillingService service(catalog, billRepo, detailRepo, nullptr, orderDetailPath);

    const Bill bill = service.generateBill(order, 0.0, 1000.0, "2026-11-24", "15:00:00");
    CHECK_EQ(bill.billId, std::string("000000001"));
    checkClose(bill.totalPayable, 820.0);
    checkClose(bill.balanceReturned, 180.0);

    const std::vector<Bill> loaded = service.loadBills();
    CHECK_EQ(loaded.size(), static_cast<std::size_t>(1));
    CHECK_EQ(loaded[0].lines.size(), static_cast<std::size_t>(2));

    removeAllTempFiles();
}