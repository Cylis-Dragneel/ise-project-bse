#include <stdexcept>
#include <string>
#include "FieldRules.h"
#include "ProductCategory.h"
#include "TestHelpers.h"

using namespace std;

int main()
{
    CHECK(FieldRules::hasWhitespace("a b"));
    CHECK(FieldRules::hasWhitespace("a\tb"));
    CHECK(!FieldRules::hasWhitespace("ab"));
    CHECK(FieldRules::hasControlChar("a\nb"));
    CHECK(FieldRules::hasControlChar(string("a\0b", 3)));
    CHECK(FieldRules::hasControlChar("a\x7F"));
    CHECK(!FieldRules::hasControlChar("Caf\xC3\xA9"));
    CHECK(FieldRules::isDigitsOnly("0123"));
    CHECK(!FieldRules::isDigitsOnly(""));
    CHECK(!FieldRules::isDigitsOnly("12a"));
    CHECK(!FieldRules::isDigitsOnly("-1"));

    CHECK_THROWS(invalid_argument, FieldRules::requireSingleWord("", "x"));
    CHECK_THROWS(invalid_argument, FieldRules::requireSingleWord("a b", "x"));
    CHECK_THROWS(invalid_argument, FieldRules::requireSingleWord("a#b", "x"));
    CHECK_THROWS(invalid_argument, FieldRules::requireSingleWord("a\x01" "b", "x"));
    FieldRules::requireSingleWord("DAIRY", "x");
    CHECK(true);

    try { FieldRules::requireSingleWord("", "category acronym"); CHECK(false); }
    catch (const invalid_argument& e)
    {
        CHECK(string(e.what()) == "category acronym must not be empty");
    }

    CHECK_THROWS(invalid_argument, FieldRules::requireDisplayName("", "x"));
    CHECK_THROWS(invalid_argument, FieldRules::requireDisplayName("A#B", "x"));
    CHECK_THROWS(invalid_argument, FieldRules::requireDisplayName("A_B", "x"));
    CHECK_THROWS(invalid_argument, FieldRules::requireDisplayName(" AB", "x"));
    CHECK_THROWS(invalid_argument, FieldRules::requireDisplayName("AB ", "x"));
    CHECK_THROWS(invalid_argument, FieldRules::requireDisplayName("A  B", "x"));
    CHECK_THROWS(invalid_argument, FieldRules::requireDisplayName("A\nB", "x"));
    FieldRules::requireDisplayName("Dairy & Breakfast", "x");
    FieldRules::requireDisplayName("Caf\xC3\xA9 Items", "x");
    CHECK(true);

    ProductCategory dairy("42", "Dairy & Breakfast", "DAIRY");
    CHECK(dairy.id() == "42");
    CHECK(dairy.name() == "Dairy & Breakfast");
    CHECK(dairy.acronym() == "DAIRY");
    CHECK(ProductCategory("10", "Low", "L").id() == "10");
    CHECK(ProductCategory("99", "High", "H").id() == "99");
    CHECK(ProductCategory("42", "X", "EIGHTCHR").acronym() == "EIGHTCHR");
    CHECK(ProductCategory("42", "Caf\xC3\xA9", "CAFE").name() == "Caf\xC3\xA9");

    const char* badIds[] = { "", "9", "1", "09", "01", "00", "100", "420",
                             "1a", "a1", " 42", "42 ", "4 2", "-5", "+5", "4.2", "4#" };
    for (size_t i = 0; i < sizeof(badIds) / sizeof(badIds[0]); i++)
    {
        bool rejected = false;
        try { ProductCategory(badIds[i], "Name", "ACR"); }
        catch (const invalid_argument&) { rejected = true; }
        if (rejected) TestHelpers::passedCount()++;
        else { TestHelpers::failedCount()++; cout << "FAILED: id '" << badIds[i] << "' was accepted" << endl; }
    }

    CHECK_THROWS(invalid_argument, ProductCategory("42", "", "ACR"));
    CHECK_THROWS(invalid_argument, ProductCategory("42", "Dairy_Items", "ACR"));
    CHECK_THROWS(invalid_argument, ProductCategory("42", "Dairy#Items", "ACR"));
    CHECK_THROWS(invalid_argument, ProductCategory("42", " Dairy", "ACR"));
    CHECK_THROWS(invalid_argument, ProductCategory("42", "Dairy ", "ACR"));
    CHECK_THROWS(invalid_argument, ProductCategory("42", "Dairy  Items", "ACR"));
    CHECK_THROWS(invalid_argument, ProductCategory("42", "Dairy\nItems", "ACR"));
    CHECK_THROWS(invalid_argument, ProductCategory("42", "Dairy\tItems", "ACR"));

    CHECK_THROWS(invalid_argument, ProductCategory("42", "Name", ""));
    CHECK_THROWS(invalid_argument, ProductCategory("42", "Name", "NINECHARS"));
    CHECK_THROWS(invalid_argument, ProductCategory("42", "Name", "A B"));
    CHECK_THROWS(invalid_argument, ProductCategory("42", "Name", "A#B"));
    CHECK_THROWS(invalid_argument, ProductCategory("42", "Name", "A\tB"));

    ProductCategory edited = dairy.withDetails("Dairy Products", "MILKY");
    CHECK(edited.id() == "42");
    CHECK(edited.name() == "Dairy Products");
    CHECK(edited.acronym() == "MILKY");
    CHECK(dairy.name() == "Dairy & Breakfast");
    CHECK_THROWS(invalid_argument, dairy.withDetails("Bad_Name", "OK"));
    CHECK_THROWS(invalid_argument, dairy.withDetails("Good", "WAYTOOLONGACR"));
    CHECK_THROWS(invalid_argument, dairy.withDetails("", "OK"));

    ProductCategory copy = dairy;
    CHECK(copy.id() == dairy.id() && copy.name() == dairy.name());

    return TestHelpers::finish("Module 0+1: FieldRules / ProductCategory");
}