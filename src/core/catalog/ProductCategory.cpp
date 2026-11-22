#include "ProductCategory.h"

#include <cctype>
#include <stdexcept>

using std::invalid_argument;
using std::string;

namespace
{
    bool hasWhitespace(const string& s)
    {
        for (size_t i = 0; i < s.size(); i++)
            if (std::isspace(static_cast<unsigned char>(s[i]))) return true;
        return false;
    }

    // Control characters (newline, tab, NUL, ...). Bytes >= 0x80 are allowed so
    // UTF-8 names still work.
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

    // A category name is stored with '_' for spaces, so '_' cannot be part of
    // the name itself; otherwise a read/write round trip would change it.
    void requireValidName(const string& name)
    {
        if (name.empty())
            throw invalid_argument("category name must not be empty");
        if (hasControlChar(name))
            throw invalid_argument("category name must not contain control characters");
        if (name.find('#') != string::npos)
            throw invalid_argument("category name must not contain '#' (it ends a record in PCategory.txt)");
        if (name.find('_') != string::npos)
            throw invalid_argument("category name must not contain '_' (it stands for a space in PCategory.txt)");
        if (name.front() == ' ' || name.back() == ' ')
            throw invalid_argument("category name must not start or end with a space");
        if (name.find("  ") != string::npos)
            throw invalid_argument("category name must not contain two spaces in a row");
    }

    void requireSingleWord(const string& value, const char* field)
    {
        if (value.empty())
            throw invalid_argument(string(field) + " must not be empty");
        if (hasWhitespace(value))
            throw invalid_argument(string(field) + " must not contain spaces (got '" + value + "')");
        if (hasControlChar(value))
            throw invalid_argument(string(field) + " must not contain control characters");
        if (value.find('#') != string::npos)
            throw invalid_argument(string(field) + " must not contain '#' (it ends a record in PCategory.txt)");
    }
}

ProductCategory::ProductCategory(const string& id,
                                 const string& name,
                                 const string& acronym)
    : id_(id), name_(name), acronym_(acronym)
{
    if (id_.size() != idLength() || !isDigitsOnly(id_))
        throw invalid_argument("category id must be exactly " +
            std::to_string(idLength()) + " digits (10-99), got '" + id_ + "'");

    const int value = (id_[0] - '0') * 10 + (id_[1] - '0');
    if (value < 10 || value > 99)
        throw invalid_argument("category id must be between 10 and 99, got '" + id_ + "'");

    requireValidName(name_);
    requireSingleWord(acronym_, "category acronym");
    if (acronym_.size() > maxAcronymLength())
        throw invalid_argument("category acronym must be at most " +
            std::to_string(maxAcronymLength()) + " characters (got '" + acronym_ + "')");
}
