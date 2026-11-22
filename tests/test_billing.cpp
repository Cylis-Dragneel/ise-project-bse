#include "test_framework.h"

#include "billing/BillCalculator.h"
#include "billing/BillDetailRecordCodec.h"
#include "billing/BillRecordCodec.h"
#include "billing/BillingService.h"
#include "billing/FileBillDetailRepository.h"
#include "billing/FileBillRepository.h"
#include "catalog/FileProductRepository.h"
#include "catalog/ProductCatalog.h"
#include "seasons/ISeasonProvider.h"
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
const char *billDetailPath = "test_billing_detail_rows_tmp.txt";
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
    std::remove(billDetailPath);
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

class FixedSeason : public ISeasonProvider
{
public:
    explicit FixedSeason(double percent) : percent_(percent) {}
    double activePercent(const std::string &) const override { return percent_; }

private:
    double percent_;
};

Order makeOrder(const std::string &orderId = "00011118",
                const std::string &status = "paid",
                double paymentAmount = 2292.5)
{
    Order order;
    order.orderId = orderId;
    order.date = "2026-11-18";
    order.time = "19:30:00";
    order.customerName = "Ali Khan";
    order.contact = "0300 1234567";
    order.status = status;
    order.paymentAmount = paymentAmount;
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

BillLine makeMilkLine(const std::string &billId = "000000001")
{
    BillLine line;
    line.billId = billId;
    line.productAcronym = "MILK1L";
    line.quantity = 2;
    line.unitPrice = 280;
    line.taxTotal = 28;
    line.discountTotal = 20;
    line.lineTotal = BillCalculator::lineTotal(line);
    return line;
}

Bill makeBill(double amountReceived = 3000)
{
    Bill bill;
    bill.billId = "000000001";
    bill.customerName = "Ali Khan";
    bill.orderId = "00011118";
    bill.paymentStatus = "paid";
    bill.billDate = "2026-11-18";
    bill.billTime = "19:30:00";
    bill.amountReceived = amountReceived;
    bill.lines.push_back(makeMilkLine());
    BillCalculator::calculate(bill);
    return bill;
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
    bill.amountReceived = 3000;
    bill.lines.push_back(makeMilkLine());
    bill.lines.push_back(makeMilkLine());

    bill.lines[1].productAcronym = "RICE5KG";
    bill.lines[1].quantity = 1;
    bill.lines[1].unitPrice = 1450;
    bill.lines[1].taxTotal = 72.5;
    bill.lines[1].discountTotal = 50;

    BillCalculator::calculate(bill);

    // Milk: 280*2 = 560, tax 28, discount 20 -> 568.
    checkClose(bill.lines[0].lineTotal, 568.0);
    // Rice: 1450, tax 72.5, discount 50 -> 1472.5.
    checkClose(bill.lines[1].lineTotal, 1472.5);

    checkClose(bill.totalNoTax, 2010.0);
    checkClose(bill.totalTax, 100.5);
    checkClose(bill.totalDiscount, 70.0);
    checkClose(bill.totalPayable, 2040.5);
    checkClose(bill.balanceReturned, 3000.0 - 2040.5);

    // The bill total is exactly the sum of the printed line totals.
    checkClose(bill.totalPayable, bill.lines[0].lineTotal + bill.lines[1].lineTotal);
}

TEST_CASE(bill_calculator_applies_seasonal_then_special)
{
    Bill bill = makeBill(3000);
    bill.lines[0].quantity = 2;

    // Base: 560 + 28 - 20 = 568.
    // Seasonal 10%: 56.80 -> 511.20.
    // Special 8% on 511.20: 40.896 -> 40.90.
    // Payable: 470.30.
    bill.specialDiscountPercent = 8;
    BillCalculator::calculate(bill, 10);

    checkClose(bill.seasonalDiscount, 56.8);
    checkClose(bill.totalPayable, 470.3);
    checkClose(bill.balanceReturned, 3000.0 - 470.3);
}

TEST_CASE(bill_calculator_rejects_an_out_of_range_special_discount)
{
    Bill bill = makeBill();
    bill.specialDiscountPercent = 8.01;
    CHECK_THROWS_AS(BillCalculator::calculate(bill, 0), std::invalid_argument);

    bill.specialDiscountPercent = -1;
    CHECK_THROWS_AS(BillCalculator::calculate(bill, 0), std::invalid_argument);

    bill.specialDiscountPercent = 0;
    CHECK_THROWS_AS(BillCalculator::calculate(bill, 101), std::invalid_argument);
    CHECK_THROWS_AS(BillCalculator::calculate(bill, -1), std::invalid_argument);
}

TEST_CASE(bill_calculator_handles_fractional_weight_quantities)
{
    BillLine sugar;
    sugar.billId = "000000001";
    sugar.productAcronym = "SUGAR";
    sugar.quantity = 4.5;
    sugar.unitPrice = 160;
    sugar.taxTotal = 36;
    sugar.discountTotal = 0;

    checkClose(BillCalculator::lineSubtotal(sugar), 720.0);
    checkClose(BillCalculator::lineTax(sugar), 36.0);
    checkClose(BillCalculator::lineDiscount(sugar), 0.0);
    checkClose(BillCalculator::lineTotal(sugar), 756.0);
}

TEST_CASE(bill_id_starts_at_one_and_increments)
{
    CHECK_EQ(BillCalculator::nextBillId({}), std::string("000000001"));

    std::vector<Bill> existing;
    existing.push_back(makeBill());
    existing.push_back(makeBill());
    existing[1].billId = "000000002";
    CHECK_EQ(BillCalculator::nextBillId(existing), std::string("000000003"));

    existing.clear();
    existing.push_back(makeBill());
    existing[0].billId = "000000042";
    CHECK_EQ(BillCalculator::nextBillId(existing), std::string("000000043"));

    existing.clear();
    existing.push_back(makeBill());
    existing[0].billId = "999999999";
    CHECK_THROWS_AS(BillCalculator::nextBillId(existing), std::overflow_error);

    existing.clear();
    existing.push_back(makeBill());
    existing[0].billId = "42";
    CHECK_THROWS_AS(BillCalculator::nextBillId(existing), std::invalid_argument);
}

TEST_CASE(bill_record_round_trips)
{
    const Bill original = makeBill();
    const std::string record = BillRecordCodec::format(original);

    CHECK(!record.empty());
    CHECK_EQ(record.back(), '#');
    CHECK_EQ(record,
             std::string("000000001 Ali_Khan 00011118 560 28 20 568 3000 2432 "
                         "paid 0 0 2026-11-18 19:30:00 #"));

    const Bill parsed = BillRecordCodec::parse(record);
    CHECK_EQ(parsed.billId, original.billId);
    CHECK_EQ(parsed.customerName, original.customerName);
    CHECK_EQ(parsed.orderId, original.orderId);
    checkClose(parsed.totalNoTax, 560);
    checkClose(parsed.totalTax, 28);
    checkClose(parsed.totalDiscount, 20);
    checkClose(parsed.totalPayable, 568);
    checkClose(parsed.amountReceived, 3000);
    checkClose(parsed.balanceReturned, 2432);
    CHECK_EQ(parsed.paymentStatus, original.paymentStatus);
    checkClose(parsed.seasonalDiscount, 0);
    checkClose(parsed.specialDiscountPercent, 0);
    CHECK_EQ(parsed.billDate, original.billDate);
    CHECK_EQ(parsed.billTime, original.billTime);

    // parse() also accepts the bare form without '#'.
    const Bill bare = BillRecordCodec::parse(record.substr(0, record.size() - 1));
    CHECK_EQ(bare.billId, original.billId);

    // An empty customer name is stored as "-".
    Bill anonymous = original;
    anonymous.customerName.clear();
    const Bill anonymousParsed = BillRecordCodec::parse(BillRecordCodec::format(anonymous));
    CHECK(anonymousParsed.customerName.empty());
}

TEST_CASE(bill_record_rejects_malformed_records)
{
    const std::string good = BillRecordCodec::format(makeBill());

    // Wrong field count.
    CHECK_THROWS_AS(BillRecordCodec::parse("000000001 Ali_Khan 00011118 560 28 20 568 3000 2432 paid 0 0 2026-11-18#"),
                    std::invalid_argument);

    // Bad bill id.
    CHECK_THROWS_AS(BillRecordCodec::parse("00000001 Ali_Khan 00011118 560 28 20 568 3000 2432 paid 0 0 2026-11-18 19:30:00#"),
                    std::invalid_argument);

    // Bad order id.
    CHECK_THROWS_AS(BillRecordCodec::parse("000000001 Ali_Khan 1118 560 28 20 568 3000 2432 paid 0 0 2026-11-18 19:30:00#"),
                    std::invalid_argument);

    // Negative money (the format does not allow a sign).
    CHECK_THROWS_AS(BillRecordCodec::parse("000000001 Ali_Khan 00011118 -560 28 20 568 3000 2432 paid 0 0 2026-11-18 19:30:00#"),
                    std::invalid_argument);

    // Unknown payment status.
    CHECK_THROWS_AS(BillRecordCodec::parse("000000001 Ali_Khan 00011118 560 28 20 568 3000 2432 refunded 0 0 2026-11-18 19:30:00#"),
                    std::invalid_argument);

    // Special discount above 8 percent.
    CHECK_THROWS_AS(BillRecordCodec::parse("000000001 Ali_Khan 00011118 560 28 20 568 3000 2432 paid 0 9 2026-11-18 19:30:00#"),
                    std::invalid_argument);

    // Bad date and time.
    CHECK_THROWS_AS(BillRecordCodec::parse("000000001 Ali_Khan 00011118 560 28 20 568 3000 2432 paid 0 0 2026-02-30 19:30:00#"),
                    std::invalid_argument);
    CHECK_THROWS_AS(BillRecordCodec::parse("000000001 Ali_Khan 00011118 560 28 20 568 3000 2432 paid 0 0 2026-11-18 25:30:00#"),
                    std::invalid_argument);

    // A record without the '#' marker at the end of the file.
    CHECK_THROWS_AS(BillRecordCodec::splitRecords(good.substr(0, good.size() - 1)),
                    std::invalid_argument);
}

TEST_CASE(bill_detail_record_round_trips)
{
    const BillLine original = makeMilkLine();
    const std::string record = BillDetailRecordCodec::format(original);

    CHECK_EQ(record, std::string("000000001 MILK1L 280 2 20 #"));

    const BillLine parsed = BillDetailRecordCodec::parse(record);
    CHECK_EQ(parsed.billId, original.billId);
    CHECK_EQ(parsed.productAcronym, original.productAcronym);
    checkClose(parsed.unitPrice, original.unitPrice);
    checkClose(parsed.quantity, original.quantity);
    checkClose(parsed.discountTotal, original.discountTotal);

    const BillLine bare = BillDetailRecordCodec::parse(record.substr(0, record.size() - 1));
    CHECK_EQ(bare.productAcronym, std::string("MILK1L"));
}

TEST_CASE(bill_detail_record_rejects_malformed_records)
{
    CHECK_THROWS_AS(BillDetailRecordCodec::parse("00000001 MILK1L 280 2 20 #"),
                    std::invalid_argument);
    CHECK_THROWS_AS(BillDetailRecordCodec::parse("000000001 MILK1L 280 0 0 #"),
                    std::invalid_argument);
    CHECK_THROWS_AS(BillDetailRecordCodec::parse("000000001 MILK1L 280 2 1000 #"),
                    std::invalid_argument);
    CHECK_THROWS_AS(BillDetailRecordCodec::parse("000000001 280 2 20 #"),
                    std::invalid_argument);
    CHECK_THROWS_AS(BillDetailRecordCodec::parse("000000001 MILK1L 280 2 #"),
                    std::invalid_argument);
}

TEST_CASE(file_bill_repositories_round_trip)
{
    std::remove(billPath);
    std::remove(billDetailPath);

    FileBillRepository billRepository(billPath);
    FileBillDetailRepository detailRepository(billDetailPath);
    CHECK(billRepository.loadAll().empty());
    CHECK(detailRepository.loadAll().empty());

    Bill first = makeBill();
    BillCalculator::calculate(first);
    Bill second = makeBill();
    second.billId = "000000002";
    second.orderId = "00021118";
    second.lines.clear();

    billRepository.saveAll({first, second});

    BillLine detail = makeMilkLine();
    BillLine detail2 = makeMilkLine("000000002");
    detail2.productAcronym = "SUGAR";
    detail2.quantity = 1.5;
    detail2.unitPrice = 160;
    detail2.taxTotal = 12;
    detail2.discountTotal = 0;
    detailRepository.saveAll({detail, detail2});

    const std::vector<Bill> loadedBills = billRepository.loadAll();
    CHECK_EQ(loadedBills.size(), static_cast<std::size_t>(2));
    CHECK_EQ(loadedBills[0].billId, std::string("000000001"));
    CHECK_EQ(loadedBills[1].billId, std::string("000000002"));

    const std::vector<BillLine> loadedDetails = detailRepository.loadAll();
    CHECK_EQ(loadedDetails.size(), static_cast<std::size_t>(2));
    CHECK_EQ(loadedDetails[1].productAcronym, std::string("SUGAR"));

    const std::string content = readFile(billPath);
    CHECK_EQ(content.back(), '\n');
    CHECK(content.find("000000001 Ali_Khan 00011118") != std::string::npos);

    const std::string detailContent = readFile(billDetailPath);
    CHECK(detailContent.find("000000001 MILK1L 280 2 20 #") != std::string::npos);

    std::remove(billPath);
    std::remove(billDetailPath);
}

TEST_CASE(billing_service_quotes_without_writing)
{
    std::remove(billPath);
    std::remove(billDetailPath);

    CatalogFixture fixture;
    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    BillingService service(fixture.catalog, billRepository, detailRepository);

    const Bill quote = service.quote(makeOrder(), makeThreeItemCart());

    CHECK(quote.billId.empty());
    CHECK_EQ(quote.orderId, std::string("00011118"));
    CHECK_EQ(quote.lines.size(), static_cast<std::size_t>(3));
    checkClose(quote.totalPayable, 2292.5);
    CHECK_EQ(quote.lines[0].productAcronym, std::string("MILK1L"));
    checkClose(quote.lines[0].discountTotal, 20.0);

    // A quote is not a bill: nothing is written and no id is consumed.
    CHECK(billRepository->loadAll().empty());
    CHECK(detailRepository->loadAll().empty());
    std::remove(billPath);
    std::remove(billDetailPath);
}

TEST_CASE(billing_service_generates_a_multi_item_bill)
{
    std::remove(billPath);
    std::remove(billDetailPath);

    CatalogFixture fixture;
    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    BillingService service(fixture.catalog, billRepository, detailRepository);

    const Order order = makeOrder();
    const Bill bill = service.generateBill(order, makeThreeItemCart());

    CHECK_EQ(bill.billId, std::string("000000001"));
    CHECK_EQ(bill.orderId, std::string("00011118"));
    CHECK_EQ(bill.customerName, std::string("Ali Khan"));
    CHECK_EQ(bill.paymentStatus, std::string("paid"));
    CHECK_EQ(bill.lines.size(), static_cast<std::size_t>(3));

    checkClose(bill.totalNoTax, 2250.0);
    checkClose(bill.totalTax, 112.5);
    checkClose(bill.totalDiscount, 70.0);
    checkClose(bill.totalPayable, 2292.5);
    checkClose(bill.amountReceived, 2292.5);
    checkClose(bill.balanceReturned, 0.0);

    CHECK_EQ(bill.lines[0].billId, std::string("000000001"));
    CHECK_EQ(bill.lines[0].productAcronym, std::string("MILK1L"));
    checkClose(bill.lines[0].unitPrice, 280.0);
    checkClose(bill.lines[0].quantity, 2.0);
    checkClose(bill.lines[0].discountTotal, 20.0);

    const std::vector<Bill> stored = service.loadBills();
    CHECK_EQ(stored.size(), static_cast<std::size_t>(1));
    CHECK_EQ(stored[0].lines.size(), static_cast<std::size_t>(3));
    checkClose(stored[0].totalPayable, 2292.5);

    std::remove(billPath);
    std::remove(billDetailPath);
}

TEST_CASE(billing_service_applies_the_seasonal_discount)
{
    std::remove(billPath);
    std::remove(billDetailPath);

    CatalogFixture fixture;
    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    FixedSeason season(10);
    BillingService service(fixture.catalog, billRepository, detailRepository, &season);

    // One milk line: base 568, seasonal 10% = 56.80 -> 511.20.
    std::vector<OrderDetail> details;
    details.push_back(makeDetail("00011118", 1, "428647", 2.0));

    const Bill quote = service.quote(makeOrder(), details, "2026-12-15", 0);
    checkClose(quote.seasonalDiscount, 56.8);
    checkClose(quote.totalPayable, 511.2);

    // Special 8% is applied after the seasonal discount:
    // 511.20 - 40.90 = 470.30.
    const Bill withSpecial = service.quote(makeOrder(), details, "2026-12-15", 8);
    checkClose(withSpecial.seasonalDiscount, 56.8);
    checkClose(withSpecial.totalPayable, 470.3);

    std::remove(billPath);
    std::remove(billDetailPath);
}

TEST_CASE(billing_service_rejects_unknown_barcodes)
{
    std::remove(billPath);
    std::remove(billDetailPath);

    CatalogFixture fixture;
    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    BillingService service(fixture.catalog, billRepository, detailRepository);

    const Order order = makeOrder();
    std::vector<OrderDetail> details;
    details.push_back(makeDetail(order.orderId, 99, "111111", 1.0));

    CHECK_THROWS_AS(service.generateBill(order, details), std::invalid_argument);

    CHECK(billRepository->loadAll().empty());
    CHECK(detailRepository->loadAll().empty());
    std::remove(billPath);
    std::remove(billDetailPath);
}

TEST_CASE(billing_service_rejects_short_stock)
{
    std::remove(billPath);
    std::remove(billDetailPath);

    CatalogFixture fixture;
    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    BillingService service(fixture.catalog, billRepository, detailRepository);

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
    Order rich = makeOrder();
    rich.paymentAmount = 100000;
    const Bill bill = service.generateBill(rich, exact);
    checkClose(bill.totalPayable, 50.0 * (280 + 14 - 10));

    std::remove(billPath);
    std::remove(billDetailPath);
}

TEST_CASE(billing_service_rejects_an_amount_below_the_total)
{
    std::remove(billPath);
    std::remove(billDetailPath);

    CatalogFixture fixture;
    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    BillingService service(fixture.catalog, billRepository, detailRepository);

    Order order = makeOrder();
    order.paymentAmount = 1.0;
    CHECK_THROWS_AS(service.generateBill(order, makeThreeItemCart()),
                    std::invalid_argument);
    CHECK(billRepository->loadAll().empty());

    std::remove(billPath);
    std::remove(billDetailPath);
}

TEST_CASE(billing_service_loads_detail_rows_from_file)
{
    removeFiles();

    writeFile(productPath, readFile(fixturePath("Product.txt")));

    ProductCatalog catalog(
        std::make_shared<FileProductRepository>(productPath));
    catalog.load();

    writeFile(detailPath,
              "00011118 1 428647 2 #\n"
              "00011118 3 579158 1.5 #\n");

    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    BillingService service(catalog, billRepository, detailRepository, nullptr, detailPath);

    const Bill bill = service.generateBill(makeOrder());

    CHECK_EQ(bill.lines.size(), static_cast<std::size_t>(2));
    checkClose(bill.totalPayable, 568.0 + 252.0);
    CHECK_EQ(detailRepository->loadAll().size(), static_cast<std::size_t>(2));

    removeFiles();
}

TEST_CASE(billing_service_generates_sequential_bill_ids)
{
    std::remove(billPath);
    std::remove(billDetailPath);

    CatalogFixture fixture;
    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    BillingService service(fixture.catalog, billRepository, detailRepository);

    const Bill first = service.generateBill(makeOrder(), makeThreeItemCart("00011118"));
    CHECK_EQ(first.billId, std::string("000000001"));

    Order secondOrder = makeOrder("00021118");
    secondOrder.paymentAmount = 5000;
    std::vector<OrderDetail> details;
    details.push_back(makeDetail("00021118", 1, "428647", 1.0));
    const Bill second = service.generateBill(secondOrder, details);
    CHECK_EQ(second.billId, std::string("000000002"));

    CHECK_EQ(service.loadBills().size(), static_cast<std::size_t>(2));
    std::remove(billPath);
    std::remove(billDetailPath);
}

TEST_CASE(billing_service_replaces_a_bill_in_place)
{
    std::remove(billPath);
    std::remove(billDetailPath);

    CatalogFixture fixture;
    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    BillingService service(fixture.catalog, billRepository, detailRepository);

    Bill bill = service.generateBill(makeOrder(), makeThreeItemCart());

    // Edit: one milk line only, same bill id.
    bill.lines.clear();
    bill.lines.push_back(BillCalculator::makeLine(
        bill.billId, makeDetail("00011118", 1, "428647", 1.0),
        *fixture.catalog.findByBarcode("428647")));
    bill.amountReceived = 1000;
    BillCalculator::calculate(bill, 0);
    service.replaceBill(bill);

    const std::vector<Bill> stored = service.loadBills();
    CHECK_EQ(stored.size(), static_cast<std::size_t>(1));
    CHECK_EQ(stored[0].billId, std::string("000000001"));
    CHECK_EQ(stored[0].lines.size(), static_cast<std::size_t>(1));
    checkClose(stored[0].totalPayable, 284.0);
    checkClose(stored[0].amountReceived, 1000.0);

    std::remove(billPath);
    std::remove(billDetailPath);
}

TEST_CASE(billing_service_rejects_duplicate_bill_ids)
{
    std::remove(billPath);
    std::remove(billDetailPath);

    writeFile(billPath,
              "000000001 Ali_Khan 00011118 560 28 20 568 3000 2432 paid 0 0 2026-11-18 19:30:00 #\n"
              "000000001 Ali_Khan 00011118 560 28 20 568 3000 2432 paid 0 0 2026-11-18 19:30:00 #\n");

    CatalogFixture fixture;
    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    BillingService service(fixture.catalog, billRepository, detailRepository);

    CHECK_THROWS_AS(service.loadBills(), std::invalid_argument);

    std::remove(billPath);
    std::remove(billDetailPath);
}

TEST_CASE(billing_end_to_end_writes_order_and_bill_files)
{
    removeFiles();

    // A complete temp data set: Product.txt, Order.txt, OrderDetail.txt,
    // Bill.txt and BillDetail.txt.
    writeFile(productPath, readFile(fixturePath("Product.txt")));

    ProductCatalog catalog(
        std::make_shared<FileProductRepository>(productPath));
    catalog.load();

    const Order order = makeOrder();
    const std::vector<OrderDetail> details = makeThreeItemCart();

    CHECK(OrderManager::saveOrderTransaction(order, details, orderPath, detailPath));

    const std::vector<Order> orders = OrderManager::loadOrders(orderPath);
    CHECK_EQ(orders.size(), static_cast<std::size_t>(1));
    CHECK_EQ(orders[0].orderId, order.orderId);
    CHECK_EQ(orders[0].status, std::string("paid"));

    auto billRepository = std::make_shared<FileBillRepository>(billPath);
    auto detailRepository = std::make_shared<FileBillDetailRepository>(billDetailPath);
    BillingService service(catalog, billRepository, detailRepository, nullptr, detailPath);
    const Bill bill = service.generateBill(order);

    checkClose(bill.totalNoTax, 2250.0);
    checkClose(bill.totalTax, 112.5);
    checkClose(bill.totalDiscount, 70.0);
    checkClose(bill.totalPayable, 2292.5);

    // Bill.txt has one '#'-terminated header line.
    const std::string billContent = readFile(billPath);
    CHECK(!billContent.empty());

    std::size_t headerCount = 0;
    std::istringstream billLines(billContent);
    std::string line;
    while (std::getline(billLines, line))
    {
        if (line.empty()) continue;
        ++headerCount;
        CHECK_EQ(line.back(), '#');

        std::istringstream fields(line);
        std::string billId, customer, orderId, noTax, tax, discount, payable,
            received, balance, status, seasonal, special, date, time, marker;
        fields >> billId >> customer >> orderId >> noTax >> tax >> discount >>
            payable >> received >> balance >> status >> seasonal >> special >>
            date >> time >> marker;

        CHECK_EQ(billId, std::string("000000001"));
        CHECK_EQ(customer, std::string("Ali_Khan"));
        CHECK_EQ(orderId, std::string("00011118"));
        CHECK_EQ(status, std::string("paid"));
        CHECK_EQ(date.size(), static_cast<std::size_t>(10)); // generated for today
        CHECK_EQ(marker, std::string("#"));
    }
    CHECK_EQ(headerCount, static_cast<std::size_t>(1));

    // BillDetail.txt has one line per item with acronym, price, quantity and
    // the whole-line discount.
    const std::string detailContent = readFile(billDetailPath);
    std::size_t detailCount = 0;
    std::istringstream detailLines(detailContent);
    while (std::getline(detailLines, line))
    {
        if (line.empty()) continue;
        ++detailCount;
        CHECK_EQ(line.back(), '#');
    }
    CHECK_EQ(detailCount, static_cast<std::size_t>(3));
    CHECK(detailContent.find("000000001 MILK1L 280 2 20 #") != std::string::npos);
    CHECK(detailContent.find("000000001 SUGAR 160 1.5 0 #") != std::string::npos);

    // Reloading joins the two files back into one bill.
    const std::vector<Bill> reloaded = service.loadBills();
    CHECK_EQ(reloaded.size(), static_cast<std::size_t>(1));
    CHECK_EQ(reloaded[0].billId, bill.billId);
    CHECK_EQ(reloaded[0].lines.size(), static_cast<std::size_t>(3));
    checkClose(reloaded[0].totalPayable, bill.totalPayable);

    removeFiles();
}
