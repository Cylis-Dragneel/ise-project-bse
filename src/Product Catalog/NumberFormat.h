#pragma once
#ifndef NUMBER_FORMAT_H
#define NUMBER_FORMAT_H

#include <sstream>
#include <iomanip>
#include <string>

// Small shared helpers so every class prints numbers the same way.
namespace NumberFormat
{
    // 150 -> "150", 2.5 -> "2.5"  (used for file storage / quantities)
    inline std::string general(double value)
    {
        std::ostringstream out;
        out << std::setprecision(12) << value;
        return out.str();
    }

    // 280 -> "280.00"  (used for money shown to the user)
    inline std::string money(double value)
    {
        std::ostringstream out;
        out << std::fixed << std::setprecision(2) << value;
        return out.str();
    }
}

#endif