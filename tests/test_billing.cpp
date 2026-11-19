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

const char *billPath = "test_billing_tmp.txt";
const char *orderPath = "test_billing_order_tmp.txt";
const char *detailPath = "test_billing_detail_tmp.txt";
const char *productPath = "test_billing_product_tmp.txt";

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

void removeFiles()
{
    std::remove(billPath);
    std::remove(orderPath);
    std::remove(detailPath);
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

Order makeOrder(const std::string &orderId = "00011118",
                const std::string &status = "paid")
{
    Order order;
    order.orderId = orderId;
    order.date = "2026-11-18";
    order.time = "19:30:00";
    order.customerName = "Ali Khan";
    order.contact = "0300 1234567";
    order.status = status;
    order.paymentAmount = 2292.5;
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

std::vector<OrderDetail> makeThreeItemCart(const std::string &orderId = "00011118")
{
    std::vector<OrderDetail> details;
    details.push_back(makeDetail(orderId, 1, "428647", 2.0));   // Milk
    details.push_back(makeDetail(orderId, 3, "579158", 1.5));   // Sugar (weight)
    details.push_back(makeDetail(orderId, 2, "358306", 1.0));   // Rice
    return details;
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

BillLine makeLine(const std::string &billId = "000000001",
                  const std::string &orderId = "00011118")
{
    BillLine line;
    line.billId = billId;
    line.orderId = orderId;
    line.productId = 1;
    line.productBarcode = "428647";
    line.quantity = 2;
    line.unitPrice = 280;
    line.taxPerUnit = 14;
    line.discountPerUnit = 10;
    line.paymentStatus = "paid";
    line.lineTotal = BillCalculator::lineTotal(line);
    return line;
}

} // namespace

TEST_CASE(bill_calculator_rounds_money_to_two_decimals)
{
    checkClose(BillCalculator::roundMoney(1.234), 1.23);
    checkClose(BillCalculator::roundMoney(1.236), 1.24);
    checkClose(BillCalculator::roundMoney(0.005), 0.01);
    checkClose(BillCalculator::roundMoney(0.0), 0.0);
    checkClose(BillCalculator::roundMoney(1500.0), 1500.0);

    // No "-0" in printed output.
    CHECK_EQ(BillCalculator::roundMoney(-0.001), 0.0);

    CHECK_THROWS_AS(BillCalculator::roundMoney(std::nan("")), std::invalid_argument);
}

TEST_CASE(bill_calculator_computes_a_multi_item_bill)
{
    Bill bill;
    bill.billId = "000000001";
    bill.orderId = "00011118";
    bill.paymentStatus = "paid";
    bill.lines.push_back(makeLine());
    bill.lines.push_back(makeLine());
    bill.lines[1].quantity = 1;
    bill.lines[1].unitPrice = 1450;
    bill.lines[1].taxPerUnit = 72.5;
    bill.lines[1].discountPerUnit = 50;

    BillCalculator::calculate(bill);

    // Milk: 280*2 = 560, tax 28, discount 20 -> 568.
    checkClose(bill.lines[0].lineTotal, 568.0);
    // Rice: 1450, tax 72.5, discount 50 -> 1472.5.
    checkClose(bill.lines[1].lineTotal, 1472.5);

    checkClose(bill.totalNoTax, 2010.0);
    checkClose(bill.totalTax, 100.5);
    checkClose(bill.totalDiscount, 70.0);
    checkClose(bill.totalPayable, 2040.5);

    // The bill total is exactly the sum of the printed line totals.
    checkClose(bill.totalPayable, bill.lines[0].lineTotal + bill.lines[1].lineTotal);
}

TEST_CASE(bill_calculator_handles_fractional_weight_quantities)
{
    BillLine sugar;
    sugar.billId = "000000001";
    sugar.orderId = "00011118";
    sugar.productId = 3;
    sugar.productBarcode = "579158";
    sugar.quantity = 4.5;
    sugar.unitPrice = 160;
    sugar.taxPerUnit = 8;
    sugar.discountPerUnit = 0;
    sugar.paymentStatus = "paid";

    checkClose(BillCalculator::lineSubtotal(sugar), 720.0);
    checkClose(BillCalculator::lineTax(sugar), 36.0);
    checkClose(BillCalculator::lineDiscount(sugar), 0.0);
    checkClose(BillCalculator::lineTotal(sugar), 756.0);
}

TEST_CASE(bill_calculator_groups_lines_into_bills)
{
    BillLine first = makeLine("000000001");
    BillLine second = makeLine("000000002");
    BillLine third = makeLine("000000001");
    third.quantity = 1;

    const std::vector<Bill> bills =
        BillCalculator::group({first, second, third});

    CHECK_EQ(bills.size(), static_cast<std::size_t>(2));
    CHECK_EQ(bills[0].billId, std::string("000000001"));
    CHECK_EQ(bills[0].lines.size(), static_cast<std::size_t>(2));
    CHECK_EQ(bills[1].billId, std::string("000000002"));
    CHECK_EQ(bills[1].lines.size(), static_cast<std::size_t>(1));
    checkClose(bills[0].totalPayable, 568.0 + 284.0);
    checkClose(bills[1].totalPayable, 568.0);
}

TEST_CASE(bill_id_starts_at_one_and_increments)
{
    CHECK_EQ(BillCalculator::nextBillId({}), std::string("000000001"));

    std::vector<BillLine> existing;
    existing.push_back(makeLine("000000001"));
    existing.push_back(makeLine("000000002"));
    existing.push_back(makeLine("000000002"));
    CHECK_EQ(BillCalculator::nextBillId(existing), std::string("000000003"));

    existing.clear();
    existing.push_back(makeLine("000000042"));
    CHECK_EQ(BillCalculator::nextBillId(existing), std::string("000000043"));

    existing.clear();
    existing.push_back(makeLine("999999999"));
    CHECK_THROWS_AS(BillCalculator::nextBillId(existing), std::overflow_error);

    existing.clear();
    existing.push_back(makeLine("42"));
    CHECK_THROWS_AS(BillCalculator::nextBillId(existing), std::invalid_argument);
}

TEST_CASE(bill_record_round_trips)
{
    const BillLine original = makeLine();
    const std::string record = BillRecordCodec::format(original);

    CHECK(!record.empty());
    CHECK_EQ(record.back(), '#');

    const BillLine parsed = BillRecordCodec::parse(record);
    CHECK_EQ(parsed.billId, original.billId);
    CHECK_EQ(parsed.orderId, original.orderId);
    CHECK_EQ(parsed.productId, original.productId);
    CHECK_EQ(parsed.productBarcode, original.productBarcode);
    checkClose(parsed.quantity, original.quantity);
    checkClose(parsed.unitPrice, original.unitPrice);
    checkClose(parsed.taxPerUnit, original.taxPerUnit);
    checkClose(parsed.discountPerUnit, original.discountPerUnit);
    checkClose(parsed.lineTotal, original.lineTotal);
    CHECK_EQ(parsed.paymentStatus, original.paymentStatus);

    // parse() also accepts the bare form without '#'.
    const BillLine bare = BillRecordCodec::parse(record.substr(0, record.size() - 1));
    CHECK_EQ(bare.billId, original.billId);
}

TEST_CASE(bill_record_rejects_malformed_records)
{
    const std::string good = BillRecordCodec::format(makeLine());

    // Wrong field count.
    CHECK_THROWS_AS(BillRecordCodec::parse("000000001 00011118 1 428647 2 280 14 10 568#"),
                    std::invalid_argument);

    // Bad bill id.
    CHECK_THROWS_AS(
        BillRecordCodec::parse("00000001 00011118 1 428647 2 280 14 10 568 paid#"),
        std::invalid_argument);

    // Bad order id.
    CHECK_THROWS_AS(
        BillRecordCodec::parse("000000001 1118 1 428647 2 280 14 10 568 paid#"),
        std::invalid_argument);

    // Barcode with a leading zero.
    CHECK_THROWS_AS(
        BillRecordCodec::parse("000000001 00011118 1 028647 2 280 14 10 568 paid#"),
        std::invalid_argument);

    // Zero quantity.
    CHECK_THROWS_AS(
        BillRecordCodec::parse("000000001 00011118 1 428647 0 280 14 10 0 paid#"),
        std::invalid_argument);

    // Negative money (the format does not allow a sign).
    CHECK_THROWS_AS(
        BillRecordCodec::parse("000000001 00011118 1 428647 2 -280 14 10 568 paid#"),
        std::invalid_argument);

    // Unknown payment status.
    CHECK_THROWS_AS(
        BillRecordCodec::parse("000000001 00011118 1 428647 2 280 14 10 568 refunded#"),
        std::invalid_argument);

    // A record without the '#' marker at the end of the file.
    CHECK_THROWS_AS(BillRecordCodec::splitRecords(good.substr(0, good.size() - 1)),
                    std::invalid_argument);
}

TEST_CASE(file_bill_repository_round_trips)
{
    std::remove(billPath);

    FileBillRepository repository(billPath);
    CHECK(repository.loadAll().empty());

    BillLine first = makeLine("000000001");
    BillLine second = makeLine("000000001");
    second.productId = 3;
    second.productBarcode = "579158";
    second.quantity = 1.5;
    second.unitPrice = 160;
    second.taxPerUnit = 8;
    second.discountPerUnit = 0;
    second.lineTotal = BillCalculator::lineTotal(second);

    repository.append({first, second});

    const std::vector<BillLine> loaded = repository.loadAll();
    CHECK_EQ(loaded.size(), static_cast<std::size_t>(2));
    CHECK_EQ(loaded[0].billId, std::string("000000001"));
    CHECK_EQ(loaded[1].productBarcode, std::string("579158"));
    checkClose(loaded[1].lineTotal, 252.0);

    const std::string content = readFile(billPath);
    CHECK(content.find('#') != std::string::npos);
    CHECK_EQ(content.back(), '\n');

    // Every record is terminated by '#'.
    std::size_t markers = 0;
    for (char character : content)
    {
        if (character == '#')
        {
            ++markers;
        }
    }
    CHECK_EQ(markers, static_cast<std::size_t>(2));

    std::remove(billPath);
}

TEST_CASE(billing_service_quotes_without_writing)
{
    std::remove(billPath);

    CatalogFixture fixture;
    auto repository = std::make_shared<FileBillRepository>(billPath);
    BillingService service(fixture.catalog, repository);

    const Bill quote = service.quote(makeOrder(), makeThreeItemCart());

    CHECK(quote.billId.empty());
    CHECK_EQ(quote.orderId, std::string("00011118"));
    CHECK_EQ(quote.lines.size(), static_cast<std::size_t>(3));
    checkClose(quote.totalPayable, 2292.5);

    // A quote is not a bill: nothing is written and no id is consumed.
    CHECK(repository->loadAll().empty());
    std::remove(billPath);
}

TEST_CASE(billing_service_generates_a_multi_item_bill)
{
    std::remove(billPath);

    CatalogFixture fixture;
    auto repository = std::make_shared<FileBillRepository>(billPath);
    BillingService service(fixture.catalog, repository);

    const Order order = makeOrder();
    const Bill bill = service.generateBill(order, makeThreeItemCart());

    CHECK_EQ(bill.billId, std::string("000000001"));
    CHECK_EQ(bill.orderId, std::string("00011118"));
    CHECK_EQ(bill.paymentStatus, std::string("paid"));
    CHECK_EQ(bill.lines.size(), static_cast<std::size_t>(3));

    checkClose(bill.totalNoTax, 2250.0);
    checkClose(bill.totalTax, 112.5);
    checkClose(bill.totalDiscount, 70.0);
    checkClose(bill.totalPayable, 2292.5);

    // The catalog is the source of truth for the line data.
    CHECK_EQ(bill.lines[0].billId, std::string("000000001"));
    CHECK_EQ(bill.lines[0].productId, 1);
    CHECK_EQ(bill.lines[0].productBarcode, std::string("428647"));
    checkClose(bill.lines[0].unitPrice, 280.0);

    const std::vector<Bill> stored = service.loadBills();
    CHECK_EQ(stored.size(), static_cast<std::size_t>(1));
    CHECK_EQ(stored[0].lines.size(), static_cast<std::size_t>(3));
    checkClose(stored[0].totalPayable, 2292.5);

    std::remove(billPath);
}

TEST_CASE(billing_service_rejects_unknown_barcodes)
{
    std::remove(billPath);

    CatalogFixture fixture;
    auto repository = std::make_shared<FileBillRepository>(billPath);
    BillingService service(fixture.catalog, repository);

    const Order order = makeOrder();
    std::vector<OrderDetail> details;
    details.push_back(makeDetail(order.orderId, 99, "111111", 1.0));

    CHECK_THROWS_AS(service.generateBill(order, details), std::invalid_argument);

    // Nothing was written.
    CHECK(repository->loadAll().empty());
    std::remove(billPath);
}

TEST_CASE(billing_service_rejects_short_stock)
{
    std::remove(billPath);

    CatalogFixture fixture;
    auto repository = std::make_shared<FileBillRepository>(billPath);
    BillingService service(fixture.catalog, repository);

    const Order order = makeOrder();

    // Soap has no stock at all.
    std::vector<OrderDetail> outOfStock;
    outOfStock.push_back(makeDetail(order.orderId, 5, "381093", 1.0));
    CHECK_THROWS_AS(service.generateBill(order, outOfStock), std::out_of_range);

    // Each row is fine on its own, but together they exceed the stock of 50.
    std::vector<OrderDetail> tooMany;
    tooMany.push_back(makeDetail(order.orderId, 1, "428647", 30.0));
    tooMany.push_back(makeDetail(order.orderId, 1, "428647", 30.0));
    CHECK_THROWS_AS(service.generateBill(order, tooMany), std::out_of_range);

    // Exactly the available amount is allowed (team rule: <=).
    std::vector<OrderDetail> exact;
    exact.push_back(makeDetail(order.orderId, 1, "428647", 50.0));
    const Bill bill = service.generateBill(order, exact);
    checkClose(bill.totalPayable, 50.0 * (280 + 14 - 10));

    std::remove(billPath);
}

TEST_CASE(billing_service_loads_detail_rows_from_file)
{
    removeFiles();

    // Copy the catalog fixture so the whole path uses temp files.
    writeFile(productPath, readFile(fixturePath("Product.txt")));

    ProductCatalog catalog(
        std::make_shared<FileProductRepository>(productPath));
    catalog.load();

    writeFile(detailPath,
              "00011118 1 428647 2 #\n"
              "00011118 3 579158 1.5 #\n");

    auto repository = std::make_shared<FileBillRepository>(billPath);
    BillingService service(catalog, repository, detailPath);

    const Bill bill = service.generateBill(makeOrder());

    CHECK_EQ(bill.lines.size(), static_cast<std::size_t>(2));
    checkClose(bill.totalPayable, 568.0 + 252.0);
    CHECK_EQ(repository->loadAll().size(), static_cast<std::size_t>(2));

    removeFiles();
}

TEST_CASE(billing_service_generates_sequential_bill_ids)
{
    std::remove(billPath);

    CatalogFixture fixture;
    auto repository = std::make_shared<FileBillRepository>(billPath);
    BillingService service(fixture.catalog, repository);

    const Order firstOrder = makeOrder("00011118");
    const Bill first = service.generateBill(firstOrder, makeThreeItemCart("00011118"));
    CHECK_EQ(first.billId, std::string("000000001"));

    const Order secondOrder = makeOrder("00021118");
    std::vector<OrderDetail> details;
    details.push_back(makeDetail("00021118", 1, "428647", 1.0));
    const Bill second = service.generateBill(secondOrder, details);
    CHECK_EQ(second.billId, std::string("000000002"));

    CHECK_EQ(service.loadBills().size(), static_cast<std::size_t>(2));
    std::remove(billPath);
}

TEST_CASE(billing_end_to_end_writes_order_and_bill_files)
{
    removeFiles();

    // A complete temp data set: Product.txt, Order.txt, OrderDetail.txt, Bill.txt.
    writeFile(productPath, readFile(fixturePath("Product.txt")));

    ProductCatalog catalog(
        std::make_shared<FileProductRepository>(productPath));
    catalog.load();

    const Order order = makeOrder();
    const std::vector<OrderDetail> details = makeThreeItemCart();

    CHECK(OrderManager::saveOrderTransaction(order, details, orderPath, detailPath));

    // The order and its rows can be read back before billing.
    const std::vector<Order> orders = OrderManager::loadOrders(orderPath);
    CHECK_EQ(orders.size(), static_cast<std::size_t>(1));
    CHECK_EQ(orders[0].orderId, order.orderId);
    CHECK_EQ(orders[0].status, std::string("paid"));
    CHECK_EQ(OrderManager::loadOrderDetails(order.orderId, detailPath).size(),
             static_cast<std::size_t>(3));

    auto repository = std::make_shared<FileBillRepository>(billPath);
    BillingService service(catalog, repository, detailPath);
    const Bill bill = service.generateBill(order);

    checkClose(bill.totalNoTax, 2250.0);
    checkClose(bill.totalTax, 112.5);
    checkClose(bill.totalDiscount, 70.0);
    checkClose(bill.totalPayable, 2292.5);

    // Bill.txt has one '#'-terminated record per item and the agreed field order.
    const std::string billContent = readFile(billPath);
    CHECK(!billContent.empty());

    std::size_t lineCount = 0;
    std::istringstream billLines(billContent);
    std::string line;
    while (std::getline(billLines, line))
    {
        if (line.empty())
        {
            continue;
        }

        ++lineCount;
        CHECK_EQ(line.back(), '#');

        std::istringstream fields(line);
        std::string billId, orderId, productId, barcode, quantity,
            unitPrice, tax, discount, total, status, marker;
        fields >> billId >> orderId >> productId >> barcode >> quantity >>
            unitPrice >> tax >> discount >> total >> status >> marker;

        CHECK_EQ(billId, std::string("000000001"));
        CHECK_EQ(orderId, std::string("00011118"));
        CHECK_EQ(status, std::string("paid"));
        CHECK_EQ(marker, std::string("#"));
    }
    CHECK_EQ(lineCount, static_cast<std::size_t>(3));

    // Reloading groups the rows back into one bill with the same totals.
    const std::vector<Bill> reloaded = service.loadBills();
    CHECK_EQ(reloaded.size(), static_cast<std::size_t>(1));
    CHECK_EQ(reloaded[0].billId, bill.billId);
    CHECK_EQ(reloaded[0].lines.size(), static_cast<std::size_t>(3));
    checkClose(reloaded[0].totalPayable, bill.totalPayable);

    removeFiles();
}
