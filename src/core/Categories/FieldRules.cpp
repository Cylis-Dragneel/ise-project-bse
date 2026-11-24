#include "FieldRules.h"
#include <cctype>
#include <stdexcept>

using std::invalid_argument;
using std::string;

namespace FieldRules
{
    bool hasWhitespace(const string& s)
    {
        for (size_t i = 0; i < s.size(); i++)
            if (std::isspace(static_cast<unsigned char>(s[i]))) return true;
        return false;
    }

    bool hasControlChar(const string& s)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (c < 0x20 || c == 0x7F) return true;
        }
        return false;
    }

    bool isDigitsOnly(const string& s)
    {
        if (s.empty()) return false;
        for (size_t i = 0; i < s.size(); i++)
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        return true;
    }

    bool equalsIgnoreCase(const string& a, const string& b)
    {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); i++)
            if (std::toupper(static_cast<unsigned char>(a[i])) !=
                std::toupper(static_cast<unsigned char>(b[i]))) return false;
        return true;
    }

    void requireSingleWord(const string& value, const string& fieldName)
    {
        if (value.empty())
            throw invalid_argument(fieldName + " must not be empty");
        if (hasWhitespace(value))
            throw invalid_argument(fieldName + " must not contain spaces (got '" + value + "')");
        if (hasControlChar(value))
            throw invalid_argument(fieldName + " must not contain control characters");
        if (value.find('#') != string::npos)
            throw invalid_argument(fieldName + " must not contain '#' (it ends a record in the data files)");
    }

    void requireDisplayName(const string& value, const string& fieldName)
    {
        if (value.empty())
            throw invalid_argument(fieldName + " must not be empty");
        if (hasControlChar(value))
            throw invalid_argument(fieldName + " must not contain control characters (e.g. new lines)");
        if (value.find('#') != string::npos)
            throw invalid_argument(fieldName + " must not contain '#' (it ends a record in the data files)");
        if (value.find('_') != string::npos)
            throw invalid_argument(fieldName + " must not contain '_' (it stands for a space in the data files)");
        if (value.front() == ' ' || value.back() == ' ')
            throw invalid_argument(fieldName + " must not start or end with a space");
        if (value.find("  ") != string::npos)
            throw invalid_argument(fieldName + " must not contain two spaces in a row");
    }
}