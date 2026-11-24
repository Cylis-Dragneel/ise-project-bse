#include <memory>
#include <stdexcept>
#include "CategoryRecordCodec.h"
#include "FileCategoryRepository.h"
#include "TestHelpers.h"
#include "TestSupport.h"

using namespace std;
using namespace TestSupport;

int main()
{
    ProductCategory p = CategoryRecordCodec::parse("42 Dairy_&_Breakfast DAIRY");
    CHECK(p.id() == "42" && p.name() == "Dairy & Breakfast" && p.acronym() == "DAIRY");
    CHECK(CategoryRecordCodec::parse("\n 35  Rice_&_Grains   GRAIN \n").acronym() == "GRAIN");
    CHECK(CategoryRecordCodec::parse("14 Cooking Essentials COOK").name() == "Cooking Essentials");
    CHECK(CategoryRecordCodec::parse("38 Care CARE").name() == "Care");

    CHECK(CategoryRecordCodec::format(p) == "42 Dairy_&_Breakfast DAIRY #");
    string line = CategoryRecordCodec::format(p);
    ProductCategory back = CategoryRecordCodec::parse(line.substr(0, line.size() - 2));
    CHECK(back.id() == p.id() && back.name() == p.name() && back.acronym() == p.acronym());

    CHECK_THROWS(invalid_argument, CategoryRecordCodec::parse(""));
    CHECK_THROWS(invalid_argument, CategoryRecordCodec::parse("42"));
    CHECK_THROWS(invalid_argument, CategoryRecordCodec::parse("42 Dairy"));
    CHECK_THROWS(invalid_argument, CategoryRecordCodec::parse("09 Dairy DAIRY"));
    CHECK_THROWS(invalid_argument, CategoryRecordCodec::parse("4 Dairy DAIRY"));
    CHECK_THROWS(invalid_argument, CategoryRecordCodec::parse("42 _Dairy DAIRY"));
    CHECK_THROWS(invalid_argument, CategoryRecordCodec::parse("42 Dairy__Items DAIRY"));
    CHECK_THROWS(invalid_argument, CategoryRecordCodec::parse("42 Dairy NINECHARS"));

    removeAll("t_cat1.txt");
    {
        FileCategoryRepository repo("t_cat1.txt");
        CHECK(repo.loadAll().empty());
        CHECK(!fileExists("t_cat1.txt"));

        vector<ProductCategory> list;
        list.push_back(ProductCategory("42", "Dairy & Breakfast", "DAIRY"));
        list.push_back(ProductCategory("35", "Rice & Grains", "GRAIN"));
        repo.saveAll(list);

        CHECK(readFile("t_cat1.txt") == "42 Dairy_&_Breakfast DAIRY #\n35 Rice_&_Grains GRAIN #\n");

        vector<ProductCategory> loaded = repo.loadAll();
        CHECK(loaded.size() == 2);
        CHECK(loaded[0].id() == "42" && loaded[0].name() == "Dairy & Breakfast");
        CHECK(loaded[1].id() == "35" && loaded[1].acronym() == "GRAIN");
        CHECK(!fileExists("t_cat1.txt.tmp") && !fileExists("t_cat1.txt.bak"));

        repo.saveAll(vector<ProductCategory>());
        CHECK(repo.loadAll().empty());
        CHECK(fileExists("t_cat1.txt"));
    }

    writeFile("t_cat2.txt", "42 Dairy DAIRY #\r\n35 Rice RICE #\r\n");
    CHECK(FileCategoryRepository("t_cat2.txt").loadAll().size() == 2);
    writeFile("t_cat3.txt", "\xEF\xBB\xBF" "42 Dairy DAIRY #");
    CHECK(FileCategoryRepository("t_cat3.txt").loadAll().size() == 1);
    writeFile("t_cat4.txt", "42 Dairy DAIRY #35 Rice RICE #");
    CHECK(FileCategoryRepository("t_cat4.txt").loadAll().size() == 2);
    writeFile("t_cat5.txt", "42 Dairy DAIRY #\n42 Other OTHER #\n");
    CHECK(FileCategoryRepository("t_cat5.txt").loadAll().size() == 2);

    writeFile("t_cat6.txt", "42 Dairy DAIRY #\n09 Bad BAD #\n");
    try { FileCategoryRepository("t_cat6.txt").loadAll(); CHECK(false); }
    catch (const invalid_argument& e)
    {
        CHECK(string(e.what()).find("record 2") != string::npos);
        CHECK(string(e.what()).find("t_cat6.txt") != string::npos);
    }
    writeFile("t_cat7.txt", "42 Dairy DAIRY\n");
    CHECK_THROWS(invalid_argument, FileCategoryRepository("t_cat7.txt").loadAll());
    writeFile("t_cat8.txt", "42 Dairy_#Items DAIRY #\n");
    CHECK_THROWS(invalid_argument, FileCategoryRepository("t_cat8.txt").loadAll());

    {
        writeFile("t_cat9.txt", "42 Dairy DAIRY #\n");
        string before = readFile("t_cat9.txt");
        auto store = make_shared<FailingStore>("t_cat9.txt", set<int>{2});
        FileCategoryRepository repo(store);
        vector<ProductCategory> list;
        list.push_back(ProductCategory("99", "New", "NEW"));
        CHECK_THROWS(runtime_error, repo.saveAll(list));
        CHECK(readFile("t_cat9.txt") == before);
        CHECK(!fileExists("t_cat9.txt.tmp") && !fileExists("t_cat9.txt.bak"));
    }
    {
        writeFile("t_cat10.txt", "42 Dairy DAIRY #\n");
        auto store = make_shared<FailingStore>("t_cat10.txt", set<int>{2, 3});
        FileCategoryRepository broken(store);
        vector<ProductCategory> list;
        list.push_back(ProductCategory("99", "New", "NEW"));
        CHECK_THROWS(runtime_error, broken.saveAll(list));
        CHECK(!fileExists("t_cat10.txt"));
        FileCategoryRepository healthy("t_cat10.txt");
        vector<ProductCategory> recovered = healthy.loadAll();
        CHECK(recovered.size() == 1 && recovered[0].id() == "42");
    }
    {
        shared_ptr<SafeFileStore> noStore;
        CHECK_THROWS(invalid_argument, FileCategoryRepository rejected(noStore));
    }

    const char* files[] = { "t_cat1.txt", "t_cat2.txt", "t_cat3.txt", "t_cat4.txt", "t_cat5.txt",
                            "t_cat6.txt", "t_cat7.txt", "t_cat8.txt", "t_cat9.txt", "t_cat10.txt" };
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) removeAll(files[i]);

    return TestHelpers::finish("Module 2: category codec / repository");
}