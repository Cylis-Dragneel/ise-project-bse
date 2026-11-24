#include <memory>
#include <stdexcept>
#include "CategoryCatalog.h"
#include "FileCategoryRepository.h"
#include "TestHelpers.h"
#include "TestSupport.h"

using namespace std;
using namespace TestSupport;

struct InMemoryCategoryRepo : ICategoryRepository
{
    vector<ProductCategory> items;
    int saves;
    InMemoryCategoryRepo() : saves(0) {}
    vector<ProductCategory> loadAll() override { return items; }
    void saveAll(const vector<ProductCategory>& c) override { items = c; saves++; }
};

static CategoryCatalog fileCatalog(const string& path)
{
    CategoryCatalog c(make_shared<FileCategoryRepository>(path));
    c.load();
    return c;
}

int main()
{
    writeFile("t_cc1.txt",
        "42 Dairy_&_Breakfast DAIRY #\n35 Rice_&_Grains GRAIN #\n57 Sugar_&_Sweeteners SUGAR #\n");
    CategoryCatalog catalog = fileCatalog("t_cc1.txt");
    CHECK(catalog.all().size() == 3);
    CHECK(catalog.findById("35") != nullptr);
    CHECK(catalog.findById("35")->name() == "Rice & Grains");
    CHECK(catalog.findById("99") == nullptr);
    CHECK(catalog.findById("") == nullptr);
    CHECK(catalog.findByAcronym("SUGAR")->id() == "57");
    CHECK(catalog.findByAcronym("sugar")->id() == "57");
    CHECK(catalog.findByAcronym("NOPE") == nullptr);

    removeAll("t_cc2.txt");
    CategoryCatalog empty = fileCatalog("t_cc2.txt");
    CHECK(empty.all().empty());
    empty.add(ProductCategory("42", "Dairy", "DAIRY"));
    CHECK(fileExists("t_cc2.txt"));
    CHECK(fileCatalog("t_cc2.txt").all().size() == 1);

    writeFile("t_cc3.txt", "42 A AA #\n42 B BB #\n");
    CHECK_THROWS(invalid_argument, fileCatalog("t_cc3.txt"));
    writeFile("t_cc3.txt", "42 A DAIRY #\n35 B dairy #\n");
    CHECK_THROWS(invalid_argument, fileCatalog("t_cc3.txt"));
    writeFile("t_cc3.txt", "42 A AA #\n35 B BB #\n");
    CHECK(fileCatalog("t_cc3.txt").all().size() == 2);

    {
        CategoryCatalog c = fileCatalog("t_cc1.txt");
        c.add(ProductCategory("14", "Cooking Essentials", "COOK"));
        CHECK(c.all().size() == 4);
        CHECK(c.findById("14")->name() == "Cooking Essentials");
        CategoryCatalog reloaded = fileCatalog("t_cc1.txt");
        CHECK(reloaded.all().size() == 4);
        CHECK(reloaded.findById("14")->acronym() == "COOK");
        CHECK(readFile("t_cc1.txt").find("14 Cooking_Essentials COOK #") != string::npos);

        string before = readFile("t_cc1.txt");
        CHECK_THROWS(invalid_argument, c.add(ProductCategory("42", "Other", "OTHER")));
        CHECK_THROWS(invalid_argument, c.add(ProductCategory("99", "Other", "DAIRY")));
        CHECK_THROWS(invalid_argument, c.add(ProductCategory("99", "Other", "dairy")));
        CHECK(c.all().size() == 4);
        CHECK(readFile("t_cc1.txt") == before);
    }

    {
        CategoryCatalog c = fileCatalog("t_cc1.txt");
        c.update("42", "Dairy Products", "MILKY");
        CHECK(c.findById("42")->name() == "Dairy Products");
        CHECK(c.findById("42")->acronym() == "MILKY");
        CHECK(c.findById("42")->id() == "42");
        CHECK(c.findByAcronym("DAIRY") == nullptr);
        CHECK(c.all()[0].id() == "42");

        CategoryCatalog reloaded = fileCatalog("t_cc1.txt");
        CHECK(reloaded.findById("42")->name() == "Dairy Products");

        c.update("42", "Dairy Products", "milky");
        CHECK(c.findById("42")->acronym() == "milky");
        c.update("42", "Dairy Products 2", "MILKY");
        CHECK(c.findById("42")->name() == "Dairy Products 2");

        string before = readFile("t_cc1.txt");
        CHECK_THROWS(invalid_argument, c.update("42", "Name", "GRAIN"));
        CHECK_THROWS(invalid_argument, c.update("42", "Bad_Name", "OK"));
        CHECK_THROWS(invalid_argument, c.update("42", "Name", "NINECHARS"));
        CHECK_THROWS(invalid_argument, c.update("42", "", "OK"));
        CHECK_THROWS(out_of_range, c.update("99", "Name", "OK"));
        CHECK(c.findById("42")->acronym() == "MILKY");
        CHECK(readFile("t_cc1.txt") == before);
    }

    {
        writeFile("t_cc4.txt", "42 Dairy DAIRY #\n");
        string before = readFile("t_cc4.txt");
        auto failing = make_shared<FailingStore>("t_cc4.txt", set<int>{2, 5});
        CategoryCatalog c(make_shared<FileCategoryRepository>(failing));
        c.load();
        CHECK_THROWS(runtime_error, c.add(ProductCategory("35", "Rice", "RICE")));
        CHECK(c.all().size() == 1);
        CHECK(c.findById("35") == nullptr);
        CHECK(readFile("t_cc4.txt") == before);

        CHECK_THROWS(runtime_error, c.update("42", "Changed", "CHG"));
        CHECK(c.findById("42")->name() == "Dairy");
        CHECK(readFile("t_cc4.txt") == before);
    }

    {
        auto fake = make_shared<InMemoryCategoryRepo>();
        fake->items.push_back(ProductCategory("42", "Dairy", "DAIRY"));
        CategoryCatalog c(fake);
        c.load();
        c.add(ProductCategory("35", "Rice", "RICE"));
        c.update("35", "Rice & Grains", "GRAIN");
        CHECK(fake->items.size() == 2);
        CHECK(fake->items[1].name() == "Rice & Grains");
        CHECK(fake->saves == 2);
        CHECK_THROWS(invalid_argument, c.add(ProductCategory("35", "X", "XX")));
        CHECK(fake->saves == 2);
    }

    {
        shared_ptr<ICategoryRepository> noRepository;
        CHECK_THROWS(invalid_argument, CategoryCatalog rejected(noRepository));
    }

    const char* files[] = { "t_cc1.txt", "t_cc2.txt", "t_cc3.txt", "t_cc4.txt" };
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) removeAll(files[i]);

    return TestHelpers::finish("Module 3: CategoryCatalog");
}