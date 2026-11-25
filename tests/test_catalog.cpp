#include "test_framework.h"

#include "catalog/FileProductRepository.h"
#include "catalog/NumberFormat.h"
#include "catalog/Product.h"
#include "catalog/ProductCatalog.h"
#include "catalog/ProductFormatter.h"
#include "catalog/ProductRecordCodec.h"

#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

const char *catalogPath = "test_catalog_tmp.txt";

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

void copyFixtureTo(const std::string &path)
{
    writeFile(path, readFile(fixturePath("Product.txt")));
}

// Fails a chosen rename so the repository's atomic-save path can be tested.
class FailingRepository : public FileProductRepository
{
public:
    explicit FailingRepository(const std::string &path)
        : FileProductRepository(path)
    {
    }

    // 0 disables the failure.
    int failOnRenameCall = 0;
    int renameCalls = 0;

protected:
    bool renameFile(const std::string &from, const std::string &to) override
    {
        ++renameCalls;
        if (failOnRenameCall != 0 && renameCalls == failOnRenameCall)
        {
            return false;
        }
        return FileProductRepository::renameFile(from, to);
    }
};

Product makeCountProduct()
{
    return Product(1, "428647", "42", "MILK1L", "Milk 1 Litre",
                   MeasurementUnit::Count, 280, 14, 10, 50,
                   "barcodes/428647.png");
}

} // namespace

TEST_CASE(catalog_loads_fixture_and_finds_products)
{
    ProductCatalog catalog(
        std::make_shared<FileProductRepository>(fixturePath("Product.txt")));
    catalog.load();

    CHECK_EQ(catalog.all().size(), static_cast<std::size_t>(4));

    const Product *milk = catalog.findByBarcode("428647");
    CHECK(milk != nullptr);
    CHECK_EQ(milk->id(), 1);
    CHECK_EQ(milk->name(), std::string("Milk 1 Litre"));
    CHECK_EQ(milk->acronym(), std::string("MILK1L"));
    CHECK(milk->unit() == MeasurementUnit::Count);
    CHECK(milk->unitPrice() == 280.0);
    CHECK(milk->taxAmount() == 14.0);
    CHECK(milk->discountPerUnit() == 10.0);
    CHECK(milk->availableQuantity() == 50.0);
    CHECK_EQ(milk->barcodeImagePath(), std::string("barcodes/428647.png"));

    // The decoder returns an int; the catalog accepts it directly.
    const Product *rice = catalog.findByBarcode(358306);
    CHECK(rice != nullptr);
    CHECK_EQ(rice->name(), std::string("Basmati Rice 5kg"));

    const Product *sugar = catalog.findById(3);
    CHECK(sugar != nullptr);
    CHECK(sugar->unit() == MeasurementUnit::Weight);
    CHECK(sugar->availableQuantity() == 35.5);

    CHECK(catalog.findByBarcode("111111") == nullptr);
    CHECK(catalog.findByBarcode(111111) == nullptr);
    CHECK(catalog.findById(999) == nullptr);
}

TEST_CASE(catalog_rejects_duplicate_ids_and_barcodes)
{
    writeFile(catalogPath,
              "1 428647 A One count 10 0 0 1 a.png#\n"
              "1 579158 B Two count 10 0 0 1 b.png#\n");

    ProductCatalog catalog(std::make_shared<FileProductRepository>(catalogPath));
    CHECK_THROWS_AS(catalog.load(), std::invalid_argument);

    writeFile(catalogPath,
              "1 428647 A One count 10 0 0 1 a.png#\n"
              "2 428647 B Two count 10 0 0 1 b.png#\n");
    CHECK_THROWS_AS(catalog.load(), std::invalid_argument);

    std::remove(catalogPath);
}

TEST_CASE(catalog_reports_record_numbers_and_missing_markers)
{
    writeFile(catalogPath,
              "1 428647 A One count 10 0 0 1 a.png#\n"
              "2 bad B Two count 10 0 0 1 b.png#\n");

    bool reportedRecord = false;
    try
    {
        ProductCatalog catalog(
            std::make_shared<FileProductRepository>(catalogPath));
        catalog.load();
    }
    catch (const std::invalid_argument &error)
    {
        reportedRecord =
            std::string(error.what()).find("record 2") != std::string::npos;
    }
    CHECK(reportedRecord);

    // A record without the '#' end marker must not be silently ignored.
    writeFile(catalogPath, "1 428647 A One count 10 0 0 1 a.png\n");
    ProductCatalog catalog(std::make_shared<FileProductRepository>(catalogPath));
    CHECK_THROWS_AS(catalog.load(), std::invalid_argument);

    std::remove(catalogPath);
}

TEST_CASE(catalog_keeps_previous_data_when_a_reload_fails)
{
    copyFixtureTo(catalogPath);

    ProductCatalog catalog(std::make_shared<FileProductRepository>(catalogPath));
    catalog.load();
    CHECK(catalog.findByBarcode("428647") != nullptr);

    writeFile(catalogPath,
              "1 428647 A One count 10 0 0 1 a.png#\n"
              "1 579158 B Two count 10 0 0 1 b.png#\n");
    CHECK_THROWS_AS(catalog.load(), std::invalid_argument);

    CHECK(catalog.findByBarcode("428647") != nullptr);
    CHECK_EQ(catalog.all().size(), static_cast<std::size_t>(4));

    std::remove(catalogPath);
}

TEST_CASE(product_rejects_invalid_fields)
{
    const MeasurementUnit count = MeasurementUnit::Count;

    CHECK_THROWS_AS(Product(0, "428647", "42", "A", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "012345", "01", "A", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "12345", "12", "A", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "1234567", "12", "A", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "12345a", "12", "A", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);

    CHECK_THROWS_AS(Product(1, "428647", "42", "ABCDEFGHI", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "42", "", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "42", "A B", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);

    CHECK_THROWS_AS(Product(1, "428647", "42", "A", "", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "42", "A", "Milk_1L", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "42", "A", "Milk#1", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "42", "A", " Milk", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "42", "A", "Milk  1L", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "42", "A", "Milk\n1L", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);

    CHECK_THROWS_AS(Product(1, "428647", "42", "A", "Name", count, -1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "42", "A", "Name", count, 1, -1, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "42", "A", "Name", count, 1, 0, 5, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(
        Product(1, "428647", "42", "A", "Name", count,
                std::numeric_limits<double>::quiet_NaN(), 0, 0, 1, "a.png"),
        std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "42", "A", "Name", count, 1, 0, 0, 1.5, "a.png"),
                    std::invalid_argument);

    // Boundary values that must be accepted.
    CHECK(Product(1, "100000", "10", "A", "Name", count, 0, 0, 0, 0, "a.png").id() == 1);

    CHECK_EQ(measurementUnitToString(MeasurementUnit::Weight), std::string("weight"));
    CHECK(measurementUnitFromString("COUNT") == MeasurementUnit::Count);
    CHECK_THROWS_AS(measurementUnitFromString("kg"), std::invalid_argument);
}

TEST_CASE(product_stock_checks_use_the_team_rule)
{
    Product product = makeCountProduct();

    // Team rule: taking exactly the available stock is allowed.
    CHECK(product.hasStockFor(50.0));
    CHECK(!product.hasStockFor(50.5));
    CHECK(!product.hasStockFor(0.0));
    CHECK(!product.hasStockFor(-1.0));
    CHECK(!product.hasStockFor(1.5));

    product.reduceStock(50.0);
    CHECK(product.availableQuantity() == 0.0);
    CHECK_THROWS_AS(product.reduceStock(1.0), std::out_of_range);

    product.addStock(10.0);
    CHECK(product.availableQuantity() == 10.0);
    CHECK_THROWS_AS(product.addStock(0.0), std::invalid_argument);

    // Weight products accept fractional quantities.
    Product sugar(3, "579158", "57", "SUGAR", "Sugar Loose", MeasurementUnit::Weight,
                  160, 8, 0, 35.5, "barcodes/579158.png");
    CHECK(sugar.hasStockFor(0.5));
    CHECK(sugar.hasStockFor(35.5));
    CHECK(!sugar.hasStockFor(35.6));
}

TEST_CASE(catalog_deduct_stock_persists_and_is_all_or_nothing)
{
    copyFixtureTo(catalogPath);

    auto repository = std::make_shared<FileProductRepository>(catalogPath);
    ProductCatalog catalog(repository);
    catalog.load();

    // Duplicate lines are added up before the stock check.
    catalog.deductStock({{"428647", 10.0}, {"428647", 5.0}});
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == 35.0);

    // The new stock is really on disk.
    ProductCatalog reloaded(std::make_shared<FileProductRepository>(catalogPath));
    reloaded.load();
    CHECK(reloaded.findByBarcode("428647")->availableQuantity() == 35.0);

    // A non-positive quantity is a bad request, not a stock shortage.
    CHECK_THROWS_AS(catalog.deductStock({{"428647", 0.0}}), std::invalid_argument);
    CHECK_THROWS_AS(catalog.deductStock({{"428647", -1.0}}), std::invalid_argument);
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == 35.0);

    // Unknown barcode: nothing changes.
    CHECK_THROWS_AS(catalog.deductStock({{"428647", 1.0}, {"111111", 1.0}}),
                    std::invalid_argument);
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == 35.0);

    // Not enough stock: nothing changes, including for the valid line.
    CHECK_THROWS_AS(catalog.deductStock({{"428647", 1.0}, {"358306", 100.0}}),
                    std::out_of_range);
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == 35.0);
    CHECK(catalog.findByBarcode("358306")->availableQuantity() == 20.0);

    // Taking the rest of the stock is allowed.
    catalog.deductStock({{"428647", 35.0}});
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == 0.0);
    CHECK(!catalog.isAvailable("428647", 1.0));

    std::remove(catalogPath);
}

TEST_CASE(catalog_rolls_back_stock_when_saving_fails)
{
    copyFixtureTo(catalogPath);
    const std::string before = readFile(catalogPath);

    auto repository = std::make_shared<FailingRepository>(catalogPath);
    ProductCatalog catalog(repository);
    catalog.load();

    // The second rename is temp -> Product.txt; failing it must undo the
    // in-memory deduction and keep the old file.
    repository->failOnRenameCall = 2;
    CHECK_THROWS_AS(catalog.deductStock({{"428647", 5.0}}), std::runtime_error);

    CHECK(catalog.findByBarcode("428647")->availableQuantity() == 50.0);
    CHECK_EQ(readFile(catalogPath), before);

    std::remove(catalogPath);
}

TEST_CASE(catalog_adjusts_stock_and_persists_changes)
{
    copyFixtureTo(catalogPath);

    ProductCatalog catalog(
        std::make_shared<FileProductRepository>(catalogPath));
    catalog.load();

    // Add and remove stock for the same product.
    catalog.adjustStock({{"428647", 10.0}, {"428647", -5.0}});
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == 55.0);

    // Weight products allow fractional stock adjustments.
    catalog.adjustStock({{"579158", 2.5}});
    CHECK(catalog.findByBarcode("579158")->availableQuantity() == 38.0);

    // Check that both changes were saved.
    ProductCatalog reloaded(
        std::make_shared<FileProductRepository>(catalogPath));
    reloaded.load();

    CHECK(reloaded.findByBarcode("428647")->availableQuantity() == 55.0);
    CHECK(reloaded.findByBarcode("579158")->availableQuantity() == 38.0);

    // Invalid adjustments must not partially change the catalog.
    CHECK_THROWS_AS(
        catalog.adjustStock({{"428647", -100.0}}),
        std::out_of_range);

    CHECK_THROWS_AS(
        catalog.adjustStock({{"428647", 1.5}}),
        std::invalid_argument);

    CHECK_THROWS_AS(
        catalog.adjustStock({{"111111", 5.0}}),
        std::invalid_argument);

    CHECK_THROWS_AS(
        catalog.adjustStock({{"428647", std::numeric_limits<double>::infinity()}}),
        std::invalid_argument);

    CHECK(catalog.findByBarcode("428647")->availableQuantity() == 55.0);

    std::remove(catalogPath);
}

TEST_CASE(catalog_adjust_stock_does_not_change_memory_when_saving_fails)
{
    copyFixtureTo(catalogPath);
    const std::string before = readFile(catalogPath);

    auto repository = std::make_shared<FailingRepository>(catalogPath);
    ProductCatalog catalog(repository);
    catalog.load();

    // Fail the second rename during the save operation.
    repository->failOnRenameCall = 2;

    CHECK_THROWS_AS(
        catalog.adjustStock({{"428647", 10.0}}),
        std::runtime_error);

    // Original in-memory stock and file must remain unchanged.
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == 50.0);
    CHECK_EQ(readFile(catalogPath), before);

    std::remove(catalogPath);
}



TEST_CASE(product_record_round_trips_through_the_codec)
{
    const Product original = makeCountProduct();

    const std::string record = ProductRecordCodec::format(original);
    CHECK(!record.empty());
    CHECK_EQ(record.back(), '#');

    // parse() accepts both the bare record and the '#'-terminated form.
    const Product parsed = ProductRecordCodec::parse(record);
    const Product bare =
        ProductRecordCodec::parse(record.substr(0, record.size() - 1));
    CHECK_EQ(bare.id(), parsed.id());

    CHECK_EQ(parsed.id(), original.id());
    CHECK_EQ(parsed.barcode(), original.barcode());
    CHECK_EQ(parsed.acronym(), original.acronym());
    CHECK_EQ(parsed.name(), original.name());
    CHECK(parsed.unit() == original.unit());
    CHECK(parsed.unitPrice() == original.unitPrice());
    CHECK(parsed.taxAmount() == original.taxAmount());
    CHECK(parsed.discountPerUnit() == original.discountPerUnit());
    CHECK(parsed.availableQuantity() == original.availableQuantity());
    CHECK_EQ(parsed.barcodeImagePath(), original.barcodeImagePath());
}

TEST_CASE(product_record_codec_splits_records)
{
    const std::vector<std::string> records = ProductRecordCodec::splitRecords(
        "1 428647 A One count 10 0 0 1 a.png#\n"
        "\n"
        "2 579158 B Two count 10 0 0 1 b.png#\n");

    CHECK_EQ(records.size(), static_cast<std::size_t>(2));
    CHECK_THROWS_AS(ProductRecordCodec::splitRecords("no marker here"),
                    std::invalid_argument);
    CHECK_EQ(ProductRecordCodec::splitRecords("").size(),
             static_cast<std::size_t>(0));
}

TEST_CASE(product_formatter_describes_a_product)
{
    const std::string text = ProductFormatter::describe(makeCountProduct());

    CHECK(text.find("Barcode       : 428647") != std::string::npos);
    CHECK(text.find("Milk 1 Litre (MILK1L)") != std::string::npos);
    CHECK(text.find("PKR 280.00") != std::string::npos);
    CHECK(text.find("In stock      : 50") != std::string::npos);
}

TEST_CASE(number_format_helpers_print_plain_decimal)
{
    CHECK_EQ(NumberFormat::general(150.0), std::string("150"));
    CHECK_EQ(NumberFormat::general(2.5), std::string("2.5"));
    CHECK_EQ(NumberFormat::general(0.0), std::string("0"));
    CHECK_EQ(NumberFormat::money(280.0), std::string("280.00"));
    CHECK_EQ(NumberFormat::money(4.756), std::string("4.76"));
}

TEST_CASE(sample_data_matches_the_committed_barcode_images)
{
    const std::string dataDir = std::string(QR_TEST_SOURCE_DIR) + "/data";

    ProductCatalog catalog(
        std::make_shared<FileProductRepository>(dataDir + "/Product.txt"));
    catalog.load();

    CHECK_EQ(catalog.all().size(), static_cast<std::size_t>(7));

    // Every image path in the sample must resolve next to Product.txt.
    for (const Product &product : catalog.all())
    {
        std::ifstream image(dataDir + "/" + product.barcodeImagePath());
        CHECK(image.good());
    }
}

TEST_CASE(product_category_must_match_the_barcode_prefix)
{
    const MeasurementUnit count = MeasurementUnit::Count;

    // Category 42 matches the 42 prefix.
    CHECK_EQ(Product(1, "428647", "42", "A", "Name", count, 1, 0, 0, 1, "a.png")
                 .categoryId(),
             std::string("42"));

    // A mismatch is rejected.
    CHECK_THROWS_AS(Product(1, "428647", "35", "A", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    // Leading-zero / malformed category ids are rejected.
    CHECK_THROWS_AS(Product(1, "428647", "02", "A", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "4", "A", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
    CHECK_THROWS_AS(Product(1, "428647", "4a", "A", "Name", count, 1, 0, 0, 1, "a.png"),
                    std::invalid_argument);
}

TEST_CASE(product_record_reads_legacy_rows_and_infers_the_category)
{
    // Increment-2 row: no category field.
    const Product legacy = ProductRecordCodec::parse(
        "1 428647 MILK1L Milk_1_Litre count 280 14 10 50 barcodes/428647.png #");

    CHECK_EQ(legacy.id(), 1);
    CHECK_EQ(legacy.barcode(), std::string("428647"));
    CHECK_EQ(legacy.categoryId(), std::string("42")); // inferred from the barcode
    CHECK_EQ(legacy.acronym(), std::string("MILK1L"));
    CHECK_EQ(legacy.name(), std::string("Milk 1 Litre"));

    // A multi-word name still works.
    const Product legacyLong = ProductRecordCodec::parse(
        "2 358306 RICE5KG Basmati Rice 5kg count 1450 72.5 50 20 b.png #");
    CHECK_EQ(legacyLong.categoryId(), std::string("35"));
    CHECK_EQ(legacyLong.name(), std::string("Basmati Rice 5kg"));
}

TEST_CASE(product_record_writes_the_category_field)
{
    const std::string record = ProductRecordCodec::format(makeCountProduct());

    // id barcode categoryId acronym ...
    CHECK(record.rfind("1 428647 42 MILK1L ", 0) == 0);

    const Product parsed = ProductRecordCodec::parse(record);
    CHECK_EQ(parsed.categoryId(), std::string("42"));
    CHECK_EQ(parsed.acronym(), std::string("MILK1L"));
}

TEST_CASE(catalog_rejects_duplicate_acronyms)
{
    writeFile(catalogPath,
              "1 428647 42 MILK1L One count 10 0 0 1 a.png#\n"
              "2 358306 35 milk1l Two count 10 0 0 1 b.png#\n");

    ProductCatalog catalog(std::make_shared<FileProductRepository>(catalogPath));
    CHECK_THROWS_AS(catalog.load(), std::invalid_argument);

    std::remove(catalogPath);
}

TEST_CASE(catalog_adds_updates_and_sets_stock)
{
    copyFixtureTo(catalogPath);

    ProductCatalog catalog(std::make_shared<FileProductRepository>(catalogPath));
    catalog.load();

    catalog.addProduct(Product(8, "420001", "42", "BUTTER", "Butter", MeasurementUnit::Count,
                               400, 20, 0, 10, "barcodes/420001.png"));
    CHECK(catalog.findById(8) != nullptr);
    CHECK(catalog.findByBarcode("420001") != nullptr);

    catalog.updateProduct(Product(8, "420001", "42", "BUTTER", "Salted Butter",
                                  MeasurementUnit::Count, 450, 22.5, 5, 10,
                                  "barcodes/420001.png"));
    CHECK_EQ(catalog.findById(8)->name(), std::string("Salted Butter"));

    catalog.setStock("420001", 4);
    CHECK(catalog.findByBarcode("420001")->availableQuantity() == 4.0);

    // Everything is persisted.
    ProductCatalog reloaded(std::make_shared<FileProductRepository>(catalogPath));
    reloaded.load();
    CHECK_EQ(reloaded.findById(8)->name(), std::string("Salted Butter"));
    CHECK(reloaded.findByBarcode("420001")->availableQuantity() == 4.0);

    // Duplicate id / barcode / acronym are rejected.
    CHECK_THROWS_AS(catalog.addProduct(Product(8, "420002", "42", "OTHER", "Other",
                                               MeasurementUnit::Count, 1, 0, 0, 1, "a.png")),
                    std::invalid_argument);
    CHECK_THROWS_AS(catalog.addProduct(Product(9, "420001", "42", "OTHER", "Other",
                                               MeasurementUnit::Count, 1, 0, 0, 1, "a.png")),
                    std::invalid_argument);
    CHECK_THROWS_AS(catalog.addProduct(Product(9, "420002", "42", "butter", "Other",
                                               MeasurementUnit::Count, 1, 0, 0, 1, "a.png")),
                    std::invalid_argument);

    // Stock cannot be set below zero or fractionally for a count product.
    CHECK_THROWS_AS(catalog.setStock("420001", -1), std::invalid_argument);
    CHECK_THROWS_AS(catalog.setStock("420001", 1.5), std::invalid_argument);
    CHECK_THROWS_AS(catalog.setStock("111111", 1), std::invalid_argument);

    std::remove(catalogPath);
}

TEST_CASE(catalog_adjust_stock_applies_mixed_movements)
{
    copyFixtureTo(catalogPath);
    ProductCatalog catalog(std::make_shared<FileProductRepository>(catalogPath));
    catalog.load();

    const double milkBefore = catalog.findByBarcode("428647")->availableQuantity();
    const double sugarBefore = catalog.findByBarcode("579158")->availableQuantity();

    // Same barcode twice is summed first: -2 + 1 = -1.
    catalog.adjustStock({{"428647", -2.0}, {"428647", 1.0}, {"579158", -1.5}});

    CHECK(catalog.findByBarcode("428647")->availableQuantity() == milkBefore - 1.0);
    CHECK(catalog.findByBarcode("579158")->availableQuantity() == sugarBefore - 1.5);

    // The new stock is persisted.
    ProductCatalog reloaded(std::make_shared<FileProductRepository>(catalogPath));
    reloaded.load();
    CHECK(reloaded.findByBarcode("428647")->availableQuantity() == milkBefore - 1.0);
    CHECK(reloaded.findByBarcode("579158")->availableQuantity() == sugarBefore - 1.5);

    std::remove(catalogPath);
}

TEST_CASE(catalog_adjust_stock_is_all_or_nothing)
{
    copyFixtureTo(catalogPath);
    ProductCatalog catalog(std::make_shared<FileProductRepository>(catalogPath));
    catalog.load();

    const double milkBefore = catalog.findByBarcode("428647")->availableQuantity();

    // Unknown barcode: nothing changes.
    CHECK_THROWS_AS(catalog.adjustStock({{"428647", 1.0}, {"999999", 1.0}}),
                    std::invalid_argument);
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == milkBefore);

    // Below zero: nothing changes.
    CHECK_THROWS_AS(catalog.adjustStock({{"428647", 1.0}, {"428647", -1e9}}),
                    std::out_of_range);
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == milkBefore);

    // A fractional movement of a count product is rejected, not rounded.
    CHECK_THROWS_AS(catalog.adjustStock({{"428647", 1.5}}), std::invalid_argument);
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == milkBefore);

    // An empty list is a successful no-op.
    catalog.adjustStock({});
    CHECK(catalog.findByBarcode("428647")->availableQuantity() == milkBefore);

    std::remove(catalogPath);
}
