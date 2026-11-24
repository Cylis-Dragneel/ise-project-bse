#pragma once
#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <iostream>

namespace TestHelpers
{
    inline int& passedCount() { static int n = 0; return n; }
    inline int& failedCount() { static int n = 0; return n; }

    inline int finish(const char* suite)
    {
        std::cout << suite << " -> Passed: " << passedCount()
            << "   Failed: " << failedCount() << std::endl;
        return failedCount() == 0 ? 0 : 1;
    }
}

#define CHECK(cond) do { if (cond) TestHelpers::passedCount()++; else { \
    TestHelpers::failedCount()++; \
    std::cout << "FAILED line " << __LINE__ << ": " #cond << std::endl; } } while (0)

#define CHECK_THROWS(E, code) do { bool ok_ = false; \
    try { code; } catch (const E &) { ok_ = true; } catch (...) {} \
    if (ok_) TestHelpers::passedCount()++; else { TestHelpers::failedCount()++; \
    std::cout << "FAILED line " << __LINE__ << ": expected " #E " from " #code << std::endl; } } while (0)

#endif