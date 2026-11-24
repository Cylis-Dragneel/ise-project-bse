#pragma once
#ifndef NUMBER_FORMAT_H
#define NUMBER_FORMAT_H

#include <sstream>
#include <iomanip>
#include <string>

namespace NumberFormat
{
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

    inline std::string money(double value)
    {
        std::ostringstream out;
        out << std::fixed << std::setprecision(2) << value;
        return out.str();
    }
}

#endif