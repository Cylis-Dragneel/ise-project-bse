#include "test_framework.h"

#include <iostream>

namespace testfw {

int runAll()
{
    int failures = 0;

    for (const TestCase &test : registry())
    {
        try
        {
            test.fn();
            std::cout << "[ PASS ] " << test.name << "\n";
        }
        catch (const AssertionFailure &e)
        {
            ++failures;
            std::cout << "[ FAIL ] " << test.name << "\n"
                      << "         " << e.what() << "\n";
        }
        catch (const std::exception &e)
        {
            ++failures;
            std::cout << "[ FAIL ] " << test.name << "\n"
                      << "         unexpected exception: " << e.what() << "\n";
        }
        catch (...)
        {
            ++failures;
            std::cout << "[ FAIL ] " << test.name << "\n"
                      << "         unexpected non-standard exception\n";
        }
    }

    std::cout << "\n"
              << (failures == 0 ? "All tests passed"
                                : std::to_string(failures) + " test(s) failed")
              << " (" << registry().size() << " test cases)\n";

    return failures == 0 ? 0 : 1;
}

} // namespace testfw

int main()
{
    return testfw::runAll();
}
