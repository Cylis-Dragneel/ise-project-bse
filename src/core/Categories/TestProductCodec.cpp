#include <stdexcept>
#include <string>
#include "Product.h"
#include "ProductRecordCodec.h"
#include "RecordSplitter.h"
#include "TestHelpers.h"

using namespace std;

static string withoutEnd(const string& line)
{
    return line.substr(0, line.size() - 2);
}

int main()
{
    Product a = ProductRecordCodec::parse("1 428647 42 MILK1L Milk_1_Litre count 280 14 10 50 barcodes/428647.png");
    CHECK(a.id() == 1 && a.barcode() == "428647" && a.categoryId() == "42");
    CHECK(a.acronym() == "MILK1L" && a.name() == "Milk 1 Litre");
    CHECK(a.unit() == MeasurementUnit::Count);
    CHECK(a.unitPrice() == 280 && a.taxAmount() == 14 && a.discountPerUnit() == 10);
    CHECK(a.availableQuantity() == 50 && a.barcodeImagePath() == "barcodes/428647.png");

    Product b = ProductRecordCodec::parse("3 579158 57 SUGAR Sugar Loose weight 160 8 0 35.5 b/3.png");
    CHECK(b.name() == "Sugar Loose" && b.categoryId() == "57" && b.availableQuantity() == 35.5);

    Product c = ProductRecordCodec::parse("\n 1  428647  42  MILK1L  Milk_1_Litre  count  280  14  10  50  p.png \n");
    CHECK(c.acronym() == "MILK1L");

    Product legacy = ProductRecordCodec::parse("1 428647 MILK1L Milk_1_Litre count 280 14 10 50 barcodes/428647.png");
    CHECK(legacy.categoryId() == "42");
    CHECK(legacy.acronym() == "MILK1L" && legacy.name() == "Milk 1 Litre");
    CHECK(legacy.availableQuantity() == 50);

    Product legacyMulti = ProductRecordCodec::parse("7 867950 ATTA Flour Atta Fine weight 140 7 0 80.5 p.png");
    CHECK(legacyMulti.categoryId() == "86" && legacyMulti.name() == "Flour Atta Fine");

    CHECK(ProductRecordCodec::parse("1 428647 42 123 Name count 1 0 0 1 p.png").acronym() == "123");
    CHECK(ProductRecordCodec::parse("1 428647 7UP Name count 1 0 0 1 p.png").acronym() == "7UP");
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 123 Name count 1 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 1 MILK1L Milk count 280 14 10 50 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 042 MILK1L Milk count 280 14 10 50 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 428647 MILK1L Milk count 280 14 10 50 p.png"));

    CHECK(ProductRecordCodec::format(a) ==
        "1 428647 42 MILK1L Milk_1_Litre count 280 14 10 50 barcodes/428647.png #");
    CHECK(ProductRecordCodec::format(b) ==
        "3 579158 57 SUGAR Sugar_Loose weight 160 8 0 35.5 b/3.png #");

    string upgraded = ProductRecordCodec::format(legacy);
    CHECK(upgraded == "1 428647 42 MILK1L Milk_1_Litre count 280 14 10 50 barcodes/428647.png #");
    CHECK(RecordSplitter::tokens(upgraded).size() == 12);

    Product back = ProductRecordCodec::parse(withoutEnd(ProductRecordCodec::format(b)));
    CHECK(back.id() == b.id() && back.barcode() == b.barcode() && back.categoryId() == b.categoryId());
    CHECK(back.acronym() == b.acronym() && back.name() == b.name() && back.unit() == b.unit());
    CHECK(back.unitPrice() == b.unitPrice() && back.taxAmount() == b.taxAmount());
    CHECK(back.discountPerUnit() == b.discountPerUnit() && back.availableQuantity() == b.availableQuantity());
    CHECK(back.barcodeImagePath() == b.barcodeImagePath());

    CHECK(RecordSplitter::split(ProductRecordCodec::format(a) + "\n" + ProductRecordCodec::format(b) + "\n").size() == 2);
    CHECK(ProductRecordCodec::splitRecords("a#b#").size() == 2);
    CHECK(ProductRecordCodec::splitRecords("").empty());
    CHECK_THROWS(invalid_argument, ProductRecordCodec::splitRecords("a#b"));

    try { ProductRecordCodec::parse("1 428647 99 MILK1L Milk count 280 14 10 50 p.png"); CHECK(false); }
    catch (const invalid_argument& e) { CHECK(string(e.what()).find("category id") != string::npos); }
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 09 MILK1L Milk count 280 14 10 50 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 4 MILK1L Milk count 280 14 10 50 p.png"));

    try { ProductRecordCodec::parse("1 428647 42 Milk count 280 14 10 50 p.png"); CHECK(false); }
    catch (const invalid_argument& e) { CHECK(string(e.what()).find("at least 11") != string::npos); }
    try { ProductRecordCodec::parse("1 428647 Milk count 280 14 10 50 p.png"); CHECK(false); }
    catch (const invalid_argument& e) { CHECK(string(e.what()).find("at least 10") != string::npos); }
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse(""));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42"));

    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name kg 1 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count abc 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count 10x 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count 0x10 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count 1e3 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count +5 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count -5 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count 1.2.3 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count inf 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count 1000000001 0 0 1 p.png"));
    CHECK(ProductRecordCodec::parse("1 428647 42 AB Name count 10 20 0 1 p.png").taxAmount() == 20);
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count 10 0 20 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count 10 0 0 2.5 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("99999999999 428647 42 AB Name count 1 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1.5 428647 42 AB Name count 1 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("-3 428647 42 AB Name count 1 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("0 428647 42 AB Name count 1 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 42864 42 AB Name count 1 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 TOOLONGACR Name count 1 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB _Name count 1 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Na__me count 1 0 0 1 p.png"));
    CHECK_THROWS(invalid_argument, ProductRecordCodec::parse("1 428647 42 AB Name count 1 0 0 1 bar#codes.png"));
    CHECK(ProductRecordCodec::parse("1 428647 42 AB Name weight 72.5 0 0 .5 p.png").availableQuantity() == 0.5);

    Product big(1, "428647", "42", "BIG", "Big Stock", MeasurementUnit::Count, 999999999, 0, 0, 123456789, "p.png");
    string bigLine = ProductRecordCodec::format(big);
    CHECK(bigLine.find("e+") == string::npos);
    Product bigBack = ProductRecordCodec::parse(withoutEnd(bigLine));
    CHECK(bigBack.unitPrice() == 999999999 && bigBack.availableQuantity() == 123456789);

    Product frac(1, "428647", "42", "FR", "Frac", MeasurementUnit::Weight, 160.75, 0.25, 0, 0.125, "p.png");
    Product fracBack = ProductRecordCodec::parse(withoutEnd(ProductRecordCodec::format(frac)));
    CHECK(fracBack.unitPrice() == 160.75 && fracBack.taxAmount() == 0.25 && fracBack.availableQuantity() == 0.125);

    return TestHelpers::finish("Module 5: product codec (10/11 fields)");
}