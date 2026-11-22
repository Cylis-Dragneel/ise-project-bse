#include "test_framework.h"

#include "catalog/CategoryCatalog.h"
#include "catalog/CategoryRecordCodec.h"
#include "catalog/FileCategoryRepository.h"
#include "catalog/FileProductRepository.h"
#include "catalog/ProductCatalog.h"
#include "catalog/ProductCategory.h"

#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

const char *categoryPath = "test_categories_tmp.txt";

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

ProductCategory makeDairy()
{
    return ProductCategory("42", "Dairy & Breakfast", "DAIRY");
}

} // namespace

TEST_CASE(category_record_round_trips)
{
    const ProductCategory original = makeDairy();
    const std::string record = CategoryRecordCodec::format(original);

    CHECK_EQ(record.back(), '#');
    CHECK(record.find("Dairy_&_Breakfast") != std::string::npos);

    const ProductCategory parsed = CategoryRecordCodec::parse(record);
    CHECK_EQ(parsed.id(), original.id());
    CHECK_EQ(parsed.name(), original.name());
    CHECK_EQ(parsed.acronym(), original.acronym());

    // The bare form without '#' is accepted too.
    const ProductCategory bare =
        CategoryRecordCodec::parse(record.substr(0, record.size() - 1));
    CHECK_EQ(bare.id(), std::string("42"));
}

TEST_CASE(category_rejects_invalid_fields)
{
    CHECK_THROWS_AS(ProductCategory("01", "Name", "A"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("09", "Name", "A"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("1", "Name", "A"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("100", "Name", "A"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("4a", "Name", "A"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("", "Name", "A"), std::invalid_argument);

    CHECK_THROWS_AS(ProductCategory("42", "", "A"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("42", "Dairy#Breakfast", "A"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("42", "Dairy_Breakfast", "A"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("42", " Dairy", "A"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("42", "Dairy  Breakfast", "A"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("42", "Dairy\nBreakfast", "A"), std::invalid_argument);

    CHECK_THROWS_AS(ProductCategory("42", "Dairy", ""), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("42", "Dairy", "A B"), std::invalid_argument);
    CHECK_THROWS_AS(ProductCategory("42", "Dairy", "ABCDEFGHI"), std::invalid_argument);

    // Boundary values that must be accepted.
    CHECK_EQ(ProductCategory("10", "Name", "ABCDEFGH").id(), std::string("10"));
    CHECK_EQ(ProductCategory("99", "Name", "A").id(), std::string("99"));
}

TEST_CASE(category_file_repository_round_trips)
{
    std::remove(categoryPath);

    FileCategoryRepository repository(categoryPath);
    CHECK(repository.loadAll().empty()); // missing file means no categories yet

    repository.saveAll({makeDairy(), ProductCategory("35", "Rice & Grains", "GRAIN")});

    const std::vector<ProductCategory> loaded = repository.loadAll();
    CHECK_EQ(loaded.size(), static_cast<std::size_t>(2));
    CHECK_EQ(loaded[0].id(), std::string("42"));
    CHECK_EQ(loaded[0].name(), std::string("Dairy & Breakfast"));
    CHECK_EQ(loaded[1].acronym(), std::string("GRAIN"));

    const std::string content = readFile(categoryPath);
    CHECK_EQ(content.back(), '\n');
    CHECK(content.find("42 Dairy_&_Breakfast DAIRY #") != std::string::npos);

    std::remove(categoryPath);
}

TEST_CASE(category_codec_splits_records)
{
    const std::vector<std::string> records = CategoryRecordCodec::splitRecords(
        "42 Dairy DAIRY #\n\n35 Grain GRAIN #\n");
    CHECK_EQ(records.size(), static_cast<std::size_t>(2));

    CHECK_THROWS_AS(CategoryRecordCodec::splitRecords("42 Dairy DAIRY"),
                    std::invalid_argument);
    CHECK_EQ(CategoryRecordCodec::splitRecords("").size(),
             static_cast<std::size_t>(0));
}

TEST_CASE(category_catalog_loads_adds_and_updates)
{
    writeFile(categoryPath,
              "42 Dairy DAIRY #\n"
              "35 Grain GRAIN #\n");

    auto repository = std::make_shared<FileCategoryRepository>(categoryPath);
    CategoryCatalog catalog(repository);
    catalog.load();

    CHECK_EQ(catalog.all().size(), static_cast<std::size_t>(2));
    CHECK(catalog.findById("42") != nullptr);
    CHECK_EQ(catalog.findById("42")->acronym(), std::string("DAIRY"));
    CHECK(catalog.findById("10") == nullptr);

    catalog.add(ProductCategory("57", "Sugar", "SUGAR"));
    CHECK(catalog.findById("57") != nullptr);

    // The new category is really on disk.
    CategoryCatalog reloaded(std::make_shared<FileCategoryRepository>(categoryPath));
    reloaded.load();
    CHECK(reloaded.findById("57") != nullptr);

    catalog.update(ProductCategory("57", "Sugar & Sweeteners", "SUGAR"));
    CHECK_EQ(catalog.findById("57")->name(), std::string("Sugar & Sweeteners"));

    // Duplicate id and duplicate acronym are rejected.
    CHECK_THROWS_AS(catalog.add(ProductCategory("42", "Other", "OTHER")),
                    std::invalid_argument);
    CHECK_THROWS_AS(catalog.add(ProductCategory("60", "Other", "dairy")),
                    std::invalid_argument);
    CHECK_THROWS_AS(catalog.update(ProductCategory("57", "Sugar", "GRAIN")),
                    std::invalid_argument);
    CHECK_THROWS_AS(catalog.update(ProductCategory("10", "Missing", "MISS")),
                    std::invalid_argument);

    std::remove(categoryPath);
}

TEST_CASE(category_catalog_rejects_duplicates_on_load)
{
    writeFile(categoryPath,
              "42 Dairy DAIRY #\n"
              "42 Other OTHER #\n");
    CategoryCatalog duplicateId(std::make_shared<FileCategoryRepository>(categoryPath));
    CHECK_THROWS_AS(duplicateId.load(), std::invalid_argument);

    writeFile(categoryPath,
              "42 Dairy DAIRY #\n"
              "35 Grain dairy #\n");
    CategoryCatalog duplicateAcronym(std::make_shared<FileCategoryRepository>(categoryPath));
    CHECK_THROWS_AS(duplicateAcronym.load(), std::invalid_argument);

    std::remove(categoryPath);
}

TEST_CASE(sample_categories_match_the_sample_products)
{
    const std::string dataDir = std::string(QR_TEST_SOURCE_DIR) + "/data";

    CategoryCatalog categories(
        std::make_shared<FileCategoryRepository>(dataDir + "/PCategory.txt"));
    categories.load();
    CHECK_EQ(categories.all().size(), static_cast<std::size_t>(7));

    ProductCatalog products(
        std::make_shared<FileProductRepository>(dataDir + "/Product.txt"));
    products.load();

    for (const Product &product : products.all())
    {
        CHECK(categories.findById(product.categoryId()) != nullptr);
        CHECK_EQ(product.barcode().substr(0, 2), product.categoryId());
    }
}
