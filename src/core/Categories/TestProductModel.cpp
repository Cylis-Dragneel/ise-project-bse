#include <stdexcept>
#include <string>
#include "Product.h"
#include "TestHelpers.h"

using namespace std;

static Product make(const string& barcode, const string& category,
    const string& acronym = "ITEM", const string& name = "Test Item",
    const string& path = "p.png", double qty = 5)
{
    return Product(1, barcode, category, acronym, name, MeasurementUnit::Count, 10, 1, 2, qty, path);
}

int main()
{
    Product milk(1, "428647", "42", "MILK1L", "Milk 1 Litre", MeasurementUnit::Count,
        280, 14, 10, 50, "barcodes/428647.png");
    CHECK(milk.id() == 1);
    CHECK(milk.barcode() == "428647");
    CHECK(milk.categoryId() == "42");
    CHECK(milk.acronym() == "MILK1L");
    CHECK(milk.name() == "Milk 1 Litre");
    CHECK(milk.unit() == MeasurementUnit::Count);
    CHECK(milk.unitPrice() == 280 && milk.taxAmount() == 14 && milk.discountPerUnit() == 10);
    CHECK(milk.availableQuantity() == 50);
    CHECK(milk.barcodeImagePath() == "barcodes/428647.png");

    CHECK(make("100000", "10").categoryId() == "10");
    CHECK(make("999999", "99").categoryId() == "99");
    CHECK(make("109999", "10").barcode() == "109999");

    CHECK_THROWS(invalid_argument, make("428647", "43"));
    CHECK_THROWS(invalid_argument, make("428647", "24"));
    CHECK_THROWS(invalid_argument, make("428647", "4"));
    CHECK_THROWS(invalid_argument, make("428647", "042"));
    CHECK_THROWS(invalid_argument, make("428647", ""));
    CHECK_THROWS(invalid_argument, make("428647", "ab"));
    CHECK_THROWS(invalid_argument, make("428647", " 4"));
    CHECK_THROWS(invalid_argument, make("428647", "4 "));
    CHECK_THROWS(invalid_argument, make("428647", "428647"));

    try { make("428647", "99"); CHECK(false); }
    catch (const invalid_argument& e)
    {
        string m = e.what();
        CHECK(m.find("category id") != string::npos);
        CHECK(m.find("428647") != string::npos);
        CHECK(m.find("42") != string::npos);
    }

    CHECK_THROWS(invalid_argument, make("42864", "42"));
    CHECK_THROWS(invalid_argument, make("4286471", "42"));
    CHECK_THROWS(invalid_argument, make("042864", "04"));
    CHECK_THROWS(invalid_argument, make("", ""));
    CHECK_THROWS(invalid_argument, make("42864A", "42"));
    CHECK_THROWS(invalid_argument, make("-42864", "-4"));

    CHECK_THROWS(invalid_argument, Product(0, "428647", "42", "AB", "Name", MeasurementUnit::Count, 1, 0, 0, 1, "p.png"));
    CHECK_THROWS(invalid_argument, Product(-1, "428647", "42", "AB", "Name", MeasurementUnit::Count, 1, 0, 0, 1, "p.png"));

    CHECK(make("428647", "42", "EIGHTCHR").acronym() == "EIGHTCHR");
    CHECK_THROWS(invalid_argument, make("428647", "42", "NINECHARS"));
    CHECK_THROWS(invalid_argument, make("428647", "42", ""));
    CHECK_THROWS(invalid_argument, make("428647", "42", "A B"));
    CHECK_THROWS(invalid_argument, make("428647", "42", "A#B"));
    CHECK_THROWS(invalid_argument, make("428647", "42", "A\tB"));

    CHECK(make("428647", "42", "AB", "Milk 1 Litre").name() == "Milk 1 Litre");
    CHECK(make("428647", "42", "AB", "Caf\xC3\xA9").name() == "Caf\xC3\xA9");
    CHECK_THROWS(invalid_argument, make("428647", "42", "AB", ""));
    CHECK_THROWS(invalid_argument, make("428647", "42", "AB", "Milk_1L"));
    CHECK_THROWS(invalid_argument, make("428647", "42", "AB", "Mi#lk"));
    CHECK_THROWS(invalid_argument, make("428647", "42", "AB", " Milk"));
    CHECK_THROWS(invalid_argument, make("428647", "42", "AB", "Milk "));
    CHECK_THROWS(invalid_argument, make("428647", "42", "AB", "Milk  1"));
    CHECK_THROWS(invalid_argument, make("428647", "42", "AB", "Milk\n1"));

    CHECK_THROWS(invalid_argument, make("428647", "42", "AB", "Name", ""));
    CHECK_THROWS(invalid_argument, make("428647", "42", "AB", "Name", "my p.png"));
    CHECK_THROWS(invalid_argument, make("428647", "42", "AB", "Name", "bar#codes.png"));

    CHECK_THROWS(invalid_argument, Product(1, "428647", "42", "AB", "N", MeasurementUnit::Count, -1, 0, 0, 1, "p"));
    CHECK_THROWS(invalid_argument, Product(1, "428647", "42", "AB", "N", MeasurementUnit::Count, 1, -1, 0, 1, "p"));
    CHECK_THROWS(invalid_argument, Product(1, "428647", "42", "AB", "N", MeasurementUnit::Count, 1, 0, -1, 1, "p"));
    CHECK_THROWS(invalid_argument, Product(1, "428647", "42", "AB", "N", MeasurementUnit::Count, 1, 0, 0, -1, "p"));
    CHECK_THROWS(invalid_argument, Product(1, "428647", "42", "AB", "N", MeasurementUnit::Count, 10, 0, 20, 1, "p"));
    CHECK_THROWS(invalid_argument, Product(1, "428647", "42", "AB", "N", MeasurementUnit::Count, 1000000001.0, 0, 0, 1, "p"));
    CHECK_THROWS(invalid_argument, Product(1, "428647", "42", "AB", "N", MeasurementUnit::Count, 1, 0, 0, 2.5, "p"));
    CHECK(Product(1, "428647", "42", "AB", "N", MeasurementUnit::Weight, 1, 0, 0, 2.5, "p").availableQuantity() == 2.5);
    CHECK(Product(1, "428647", "42", "AB", "N", MeasurementUnit::Count, 10, 0, 10, 1, "p").discountPerUnit() == 10);

    Product stock = make("428647", "42", "AB", "Name", "p.png", 10);
    CHECK(stock.hasStockFor(10));
    CHECK(!stock.hasStockFor(11));
    CHECK(!stock.hasStockFor(0));
    CHECK(!stock.hasStockFor(-1));
    CHECK(!stock.hasStockFor(1.5));
    stock.reduceStock(4);
    CHECK(stock.availableQuantity() == 6);
    CHECK_THROWS(out_of_range, stock.reduceStock(7));
    CHECK(stock.availableQuantity() == 6);
    stock.addStock(4);
    CHECK(stock.availableQuantity() == 10);
    CHECK_THROWS(invalid_argument, stock.addStock(0));
    CHECK_THROWS(invalid_argument, stock.addStock(1.5));
    CHECK_THROWS(invalid_argument, stock.addStock(Product::maxValue()));

    CHECK(measurementUnitFromString("count") == MeasurementUnit::Count);
    CHECK(measurementUnitFromString("WEIGHT") == MeasurementUnit::Weight);
    CHECK_THROWS(invalid_argument, measurementUnitFromString("kg"));
    CHECK(toString(MeasurementUnit::Count) == "count");
    CHECK(toString(MeasurementUnit::Weight) == "weight");

    return TestHelpers::finish("Module 4: Product + categoryId");
}