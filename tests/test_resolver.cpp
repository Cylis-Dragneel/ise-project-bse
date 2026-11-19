#include "test_framework.h"

#include "catalog/FileProductRepository.h"
#include "catalog/ProductCatalog.h"
#include "catalog/ProductResolver.h"

#include <limits>
#include <memory>
#include <string>

namespace {

std::string fixturePath(const char *name)
{
    return std::string(QR_TEST_SOURCE_DIR) + "/tests/fixtures/" + name;
}

struct Fixture
{
    Fixture()
        : catalog(std::make_shared<FileProductRepository>(
              fixturePath("Product.txt"))),
          resolver(catalog)
    {
        catalog.load();
    }

    ProductCatalog catalog;
    ProductResolver resolver;
};

} // namespace

TEST_CASE(resolver_finds_a_product_by_barcode)
{
    Fixture fixture;

    const ProductResolver::Result result = fixture.resolver.lookup("428647");
    CHECK(result.found());
    CHECK(result.status == ProductResolver::Status::Found);
    CHECK(result.product != nullptr);
    CHECK_EQ(result.product->name(), std::string("Milk 1 Litre"));
    CHECK(result.message.empty());
    CHECK(result.description.find("Milk 1 Litre") != std::string::npos);
    CHECK(result.description.find("Barcode       : 428647") != std::string::npos);
}

TEST_CASE(resolver_ignores_surrounding_whitespace)
{
    Fixture fixture;

    CHECK(fixture.resolver.lookup(" 428647 ").found());
    CHECK(fixture.resolver.lookup("\t428647\r\n").found());
    CHECK(fixture.resolver.lookup("358306").found());
}

TEST_CASE(resolver_rejects_invalid_barcodes)
{
    Fixture fixture;

    const char *invalid[] = {"", "   ", "12345", "1234567", "012345", "abcdef",
                             "42864x"};

    for (const char *barcode : invalid)
    {
        const ProductResolver::Result result = fixture.resolver.lookup(barcode);
        CHECK(result.status == ProductResolver::Status::InvalidBarcode);
        CHECK(result.product == nullptr);
        CHECK(result.description.empty());
        CHECK(!result.message.empty());
    }
}

TEST_CASE(resolver_reports_unknown_barcodes)
{
    Fixture fixture;

    const ProductResolver::Result result = fixture.resolver.lookup("111111");
    CHECK(result.status == ProductResolver::Status::UnknownBarcode);
    CHECK(result.product == nullptr);
    CHECK(result.description.empty());
    CHECK(result.message.find("111111") != std::string::npos);
}

TEST_CASE(resolver_checks_quantity_against_stock)
{
    Fixture fixture;

    // Exactly the available stock is allowed (team rule).
    CHECK(fixture.resolver.lookupForQuantity("428647", 50.0).found());

    const ProductResolver::Result tooMany =
        fixture.resolver.lookupForQuantity("428647", 50.5);
    CHECK(tooMany.status == ProductResolver::Status::InsufficientStock);
    // The product is still known, so the UI can show the specs.
    CHECK(tooMany.product != nullptr);
    CHECK(!tooMany.description.empty());
    CHECK(tooMany.message.find("50.5") != std::string::npos);

    // A bad quantity is refused as well.
    CHECK(fixture.resolver.lookupForQuantity("428647", 0.0).status ==
          ProductResolver::Status::InsufficientStock);
    CHECK(fixture.resolver.lookupForQuantity("428647", -1.0).status ==
          ProductResolver::Status::InsufficientStock);
    CHECK(fixture.resolver
              .lookupForQuantity("428647",
                                 std::numeric_limits<double>::quiet_NaN())
              .status == ProductResolver::Status::InsufficientStock);

    // Weight products accept fractions.
    CHECK(fixture.resolver.lookupForQuantity("579158", 0.5).found());
    CHECK(fixture.resolver.lookupForQuantity("579158", 36.0).status ==
          ProductResolver::Status::InsufficientStock);

    // Unknown and invalid barcodes keep their own status.
    CHECK(fixture.resolver.lookupForQuantity("111111", 1.0).status ==
          ProductResolver::Status::UnknownBarcode);
    CHECK(fixture.resolver.lookupForQuantity("bad", 1.0).status ==
          ProductResolver::Status::InvalidBarcode);
}

TEST_CASE(resolver_result_matches_the_sample_catalog)
{
    Fixture fixture;

    // Every product in the fixture must be reachable by its own barcode.
    for (const Product &product : fixture.catalog.all())
    {
        const ProductResolver::Result result =
            fixture.resolver.lookup(product.barcode());
        CHECK(result.found());
        CHECK(result.product != nullptr);
        CHECK_EQ(result.product->id(), product.id());
    }
}
