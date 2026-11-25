#include "test_framework.h"

#include "billing/BillRecordCodec.h"
#include "catalog/FileProductRepository.h"
#include "catalog/ProductCatalog.h"
#include "reports/ReportsService.h"

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

namespace {

const char *productPath = "reports_products_tmp.txt";
const char *orderPath = "reports_orders_tmp.txt";
const char *detailPath = "reports_order_details_tmp.txt";
const char *billPath = "reports_bills_tmp.txt";
const char *billDetailPath = "reports_bill_details_tmp.txt";

void writeFile(const char *path, const std::string &content)
{
    std::ofstream file(path, std::ios::trunc);
    file << content;
}

void removeFiles()
{
    std::remove(productPath);
    std::remove(orderPath);
    std::remove(detailPath);
    std::remove(billPath);
    std::remove(billDetailPath);
}

struct ReportFixture
{
    ReportFixture()
    {
        removeFiles();
        writeFile(productPath,
                  "1 428647 42 MILK1L Milk_1_Litre count 280 14 10 50 "
                  "barcodes/428647.png #\n");
        writeFile(orderPath,
                  "00011125 2026-11-25 09:00:00 Open_Customer - in-progress "
                  "0 cash - #\n"
                  "00021125 2026-11-25 10:00:00 Paid_Customer - paid 100 cash - #\n");
        writeFile(detailPath,
                  "00011125 1 428647 2 #\n"
                  "00021125 1 428647 1 #\n");
        writeFile(billPath,
                  "000000001 Paid_Customer 00021125 280 14 10 284 "
                  "300 16 paid 0 0 2026-11-25 10:01:00 #\n");
        writeFile(billDetailPath, "");
    }

    ~ReportFixture()
    {
        removeFiles();
    }

    ProductCatalog catalog()
    {
        ProductCatalog result(
            std::make_shared<FileProductRepository>(productPath));
        result.load();
        return result;
    }
};

} // namespace

TEST_CASE(reports_separate_incomplete_quotes_from_completed_bill_totals)
{
    ReportFixture fixture;
    ProductCatalog catalog = fixture.catalog();
    reports::ReportsService service(catalog, orderPath, detailPath, billPath,
                                    billDetailPath);

    double incompleteTotal = 0;
    const std::vector<reports::OrderRow> incomplete =
        service.incompleteOrders(&incompleteTotal);
    CHECK_EQ(incomplete.size(), static_cast<std::size_t>(1));
    CHECK_EQ(incomplete[0].orderId, std::string("00011125"));
    CHECK_EQ(incomplete[0].status, std::string("in-progress"));
    CHECK_EQ(incomplete[0].total, 568.0);
    CHECK_EQ(incompleteTotal, 568.0);

    double completedTotal = 0;
    const std::vector<reports::OrderRow> completed =
        service.completedOrders(&completedTotal);
    CHECK_EQ(completed.size(), static_cast<std::size_t>(1));
    CHECK_EQ(completed[0].orderId, std::string("00021125"));
    CHECK_EQ(completed[0].total, 284.0);
    CHECK_EQ(completedTotal, 284.0);
}

TEST_CASE(reports_group_cash_sales_and_rank_sales_data)
{
    ReportFixture fixture;
    ProductCatalog catalog = fixture.catalog();

    writeFile(orderPath,
              "00011123 2026-11-23 09:00:00 A - paid 0 cash - #\n"
              "00021124 2026-11-24 09:00:00 B - paid 0 cash - #\n"
              "00031125 2026-11-25 09:00:00 C - paid 0 card - #\n"
              "00041125 2026-11-25 09:00:00 D - paid 0 cash - #\n");
    writeFile(detailPath,
              "00011123 1 428647 1 #\n"
              "00021124 1 428647 2 #\n"
              "00031125 1 428647 3 #\n"
              "00041125 1 428647 4 #\n");
    writeFile(billPath,
              "000000001 A 00011123 10 0 1 10 10 0 paid 0 0 2026-11-23 09:01:00 #\n"
              "000000002 B 00021124 20 0 2 20 20 0 paid 0 0 2026-11-24 09:01:00 #\n"
              "000000003 C 00031125 30 0 3 30 30 0 paid 0 0 2026-11-25 09:01:00 #\n"
              "000000004 D 00041125 40 0 4 40 40 0 paid 0 8 2026-11-25 09:02:00 #\n");
    writeFile(billDetailPath,
              "000000001 MILK1L 280 1 10 #\n"
              "000000002 MILK1L 280 2 20 #\n"
              "000000003 MILK1L 280 3 30 #\n"
              "000000004 MILK1L 280 4 40 #\n");

    reports::ReportsService service(catalog, orderPath, detailPath, billPath,
                                    billDetailPath);

    double weeklyTotal = 0;
    const std::vector<reports::CashRow> weekly =
        service.cashSales(reports::CashPeriod::Weekly, "", "", "2026-11-25",
                          &weeklyTotal);
    CHECK_EQ(weekly.size(), static_cast<std::size_t>(3));
    CHECK_EQ(weekly[0].period, std::string("2026-11-23"));
    CHECK_EQ(weekly[0].amount, 10.0);
    CHECK_EQ(weekly[1].period, std::string("2026-11-24"));
    CHECK_EQ(weekly[1].amount, 20.0);
    CHECK_EQ(weekly[2].period, std::string("2026-11-25"));
    CHECK_EQ(weekly[2].amount, 40.0);
    CHECK_EQ(weeklyTotal, 70.0);

    const std::vector<reports::CashRow> yearly =
        service.cashSales(reports::CashPeriod::Yearly, "", "", "2026-11-25");
    CHECK_EQ(yearly.size(), static_cast<std::size_t>(1));
    CHECK_EQ(yearly[0].period, std::string("2026-11"));
    CHECK_EQ(yearly[0].amount, 70.0);

    const std::vector<reports::CashRow> custom =
        service.cashSales(reports::CashPeriod::Custom, "2026-11-24",
                          "2026-11-25", "2026-11-25");
    CHECK_EQ(custom.size(), static_cast<std::size_t>(2));
    CHECK_EQ(custom[0].amount, 20.0);
    CHECK_EQ(custom[1].amount, 40.0);

    reports::SalesTotals totals;
    const std::vector<reports::SalesRow> sales = service.allSales(&totals);
    CHECK_EQ(sales.size(), static_cast<std::size_t>(4));
    CHECK_EQ(sales[3].specialDiscount, 2.88);
    CHECK_EQ(totals.productDiscount, 10.0);
    CHECK_EQ(totals.seasonalDiscount, 0.0);
    CHECK_EQ(totals.specialDiscount, 2.88);
    CHECK_EQ(totals.totalPayable, 100.0);

    const std::vector<reports::ItemRow> sold = service.topSoldItems(1);
    CHECK_EQ(sold.size(), static_cast<std::size_t>(1));
    CHECK_EQ(sold[0].acronym, std::string("MILK1L"));
    CHECK_EQ(sold[0].quantity, 10.0);
    CHECK_EQ(sold[0].remainingStock, 50.0);

    const std::vector<reports::ItemRow> stock = service.topStockItems(1);
    CHECK_EQ(stock.size(), static_cast<std::size_t>(1));
    CHECK_EQ(stock[0].acronym, std::string("MILK1L"));
    CHECK_EQ(stock[0].remainingStock, 50.0);
}
