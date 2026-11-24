#include <stdexcept>
#include "FieldRules.h"
#include "RecordSplitter.h"
#include "SafeFileStore.h"
#include "TestHelpers.h"
#include "TestSupport.h"

using namespace std;
using namespace TestSupport;

int main()
{
    CHECK(FieldRules::equalsIgnoreCase("Dairy", "DAIRY"));
    CHECK(FieldRules::equalsIgnoreCase("", ""));
    CHECK(!FieldRules::equalsIgnoreCase("Dairy", "Dairy1"));
    CHECK(!FieldRules::equalsIgnoreCase("abc", "abd"));

    CHECK(RecordSplitter::split("").empty());
    CHECK(RecordSplitter::split("  \n\n").empty());
    CHECK(RecordSplitter::split("a#b#").size() == 2);
    CHECK(RecordSplitter::split("a#b#\n c #").size() == 3);
    CHECK(RecordSplitter::split("a#  \n").size() == 1);
    CHECK(RecordSplitter::split("##a##").size() == 1);
    CHECK(RecordSplitter::split("a#b#")[1] == "b");
    CHECK_THROWS(invalid_argument, RecordSplitter::split("a#b"));
    CHECK_THROWS(invalid_argument, RecordSplitter::split("a# b"));
    CHECK_THROWS(invalid_argument, RecordSplitter::split("abc"));

    CHECK(RecordSplitter::tokens("  a   b\tc\n").size() == 3);
    CHECK(RecordSplitter::tokens("").empty());
    CHECK(RecordSplitter::tokens("a b c")[2] == "c");
    CHECK(RecordSplitter::endRecord("x y") == "x y #");
    CHECK(RecordSplitter::split(RecordSplitter::endRecord("x y") + "\n").size() == 1);

    removeAll("t_s1.txt");
    SafeFileStore plain("t_s1.txt");
    CHECK(plain.path() == "t_s1.txt");
    CHECK(!plain.exists());
    CHECK_THROWS(runtime_error, plain.read());

    plain.write("hello\nworld\n");
    CHECK(plain.exists());
    CHECK(plain.read() == "hello\nworld\n");
    plain.write("second");
    CHECK(plain.read() == "second");
    plain.write("");
    CHECK(plain.read() == "");
    CHECK(!fileExists("t_s1.txt.tmp") && !fileExists("t_s1.txt.bak"));

    writeFile("t_bom.txt", "\xEF\xBB\xBF" "data");
    CHECK(SafeFileStore("t_bom.txt").read() == "data");

    {
        writeFile("t_s2.txt", "original");
        FailingStore store("t_s2.txt", { 1 });
        CHECK_THROWS(runtime_error, store.write("new"));
        CHECK(readFile("t_s2.txt") == "original");
        CHECK(!fileExists("t_s2.txt.tmp") && !fileExists("t_s2.txt.bak"));
    }
    {
        writeFile("t_s3.txt", "original");
        FailingStore store("t_s3.txt", { 2 });
        try { store.write("new"); CHECK(false); }
        catch (const runtime_error& e) { CHECK(string(e.what()).find("restored") != string::npos); }
        CHECK(readFile("t_s3.txt") == "original");
        CHECK(!fileExists("t_s3.txt.tmp") && !fileExists("t_s3.txt.bak"));
    }
    {
        writeFile("t_s4.txt", "original");
        FailingStore store("t_s4.txt", { 2, 3 });
        try { store.write("new"); CHECK(false); }
        catch (const runtime_error& e) { CHECK(string(e.what()).find("t_s4.txt.bak") != string::npos); }
        CHECK(!fileExists("t_s4.txt") && fileExists("t_s4.txt.bak"));
        SafeFileStore healthy("t_s4.txt");
        CHECK(healthy.exists());
        CHECK(healthy.read() == "original");
        CHECK(fileExists("t_s4.txt") && !fileExists("t_s4.txt.bak"));
    }
    {
        writeFile("t_s5.txt", "original");
        writeFile("t_s5.txt.bak", "stale");
        SafeFileStore store("t_s5.txt");
        store.write("fresh");
        CHECK(store.read() == "fresh");
        CHECK(!fileExists("t_s5.txt.bak"));
    }
    {
        removeAll("t_s6.txt");
        FailingStore store("t_s6.txt", { 1 });
        CHECK_THROWS(runtime_error, store.write("first save"));
        CHECK(!fileExists("t_s6.txt"));
        CHECK(!fileExists("t_s6.txt.tmp") && !fileExists("t_s6.txt.bak"));
        SafeFileStore healthy("t_s6.txt");
        healthy.write("first save");
        CHECK(healthy.read() == "first save");
    }

    const char* files[] = { "t_s1.txt", "t_bom.txt", "t_s2.txt", "t_s3.txt", "t_s4.txt", "t_s5.txt", "t_s6.txt" };
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) removeAll(files[i]);

    return TestHelpers::finish("Module 0: FieldRules+ / RecordSplitter / SafeFileStore");
}