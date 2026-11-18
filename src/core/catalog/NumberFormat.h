#ifndef NUMBER_FORMAT_H
#define NUMBER_FORMAT_H

#include <sstream>
#include <iomanip>
#include <string>

// Small shared helpers so every class prints numbers the same way.
namespace NumberFormat
{
    // 150 -> "150", 2.5 -> "2.5". Plain decimal notation only (never "1e+12"),
    // so whatever is written to the file can always be read back.
    inline std::string general(double value)
    {
        std::ostringstream out;
        out << std::fixed << std::setprecision(6) << value;
        std::string s = out.str();
        if (s.find('.') != std::string::npos)
        {
            while (s.back() == '0') s.pop_back();
            if (s.back() == '.') s.pop_back();
        }
        if (s == "-0") s = "0";
        return s;
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