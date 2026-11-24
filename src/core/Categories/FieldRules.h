#ifndef FIELD_RULES_H
#define FIELD_RULES_H

#include <string>

namespace FieldRules
{
    bool hasWhitespace(const std::string& s);
    bool hasControlChar(const std::string& s);
    bool isDigitsOnly(const std::string& s);
    bool equalsIgnoreCase(const std::string& a, const std::string& b);

    void requireSingleWord(const std::string& value, const std::string& fieldName);

    void requireDisplayName(const std::string& value, const std::string& fieldName);
}

#endif