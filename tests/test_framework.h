#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace testfw {

struct TestCase
{
    const char *name;
    void (*fn)();
};

inline std::vector<TestCase> &registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar
{
    Registrar(const char *name, void (*fn)())
    {
        registry().push_back({name, fn});
    }
};

struct AssertionFailure : std::runtime_error
{
    explicit AssertionFailure(const std::string &message)
        : std::runtime_error(message)
    {
    }
};

template <typename Actual, typename Expected>
void checkEqual(const Actual &actual, const Expected &expected,
                const char *actualExpr, const char *expectedExpr,
                const char *file, int line)
{
    if (actual == expected)
    {
        return;
    }

    std::ostringstream message;
    message << file << ":" << line
            << ": expected " << actualExpr << " == " << expectedExpr
            << " (got " << actual << ", expected " << expected << ")";
    throw AssertionFailure(message.str());
}

template <typename Exception, typename Fn>
void checkThrows(Fn &&fn, const char *expr, const char *file, int line)
{
    try
    {
        fn();
    }
    catch (const Exception &)
    {
        return;
    }
    catch (const std::exception &e)
    {
        std::ostringstream message;
        message << file << ":" << line << ": " << expr
                << " threw the wrong exception: " << e.what();
        throw AssertionFailure(message.str());
    }
    catch (...)
    {
        std::ostringstream message;
        message << file << ":" << line << ": " << expr
                << " threw a non-standard exception";
        throw AssertionFailure(message.str());
    }

    std::ostringstream message;
    message << file << ":" << line << ": " << expr << " did not throw";
    throw AssertionFailure(message.str());
}

// Defined in test_main.cpp.
int runAll();

} // namespace testfw

#define TEST_CASE(name)                                     \
    static void name();                                     \
    static ::testfw::Registrar name##_registrar(#name, &name); \
    static void name()

#define CHECK(expr)                                                          \
    do                                                                       \
    {                                                                        \
        if (!(expr))                                                         \
        {                                                                    \
            throw ::testfw::AssertionFailure(                                \
                std::string(__FILE__) + ":" + std::to_string(__LINE__) +     \
                ": CHECK failed: " #expr);                                   \
        }                                                                    \
    } while (false)

#define CHECK_EQ(actual, expected) \
    ::testfw::checkEqual((actual), (expected), #actual, #expected, __FILE__, __LINE__)

#define CHECK_THROWS_AS(expr, Exception) \
    ::testfw::checkThrows<Exception>([&]() { expr; }, #expr, __FILE__, __LINE__)

#endif
