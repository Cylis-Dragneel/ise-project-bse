#include "ProductCategory.h"
#include "FieldRules.h"
#include <stdexcept>

using std::invalid_argument;
using std::string;

ProductCategory::ProductCategory(const string& id, const string& name, const string& acronym)
    : id_(id), name_(name), acronym_(acronym)
{
    if (!FieldRules::isDigitsOnly(id_) || id_.size() != idLength() || id_[0] == '0')
        throw invalid_argument("category id must be 2 digits from 10 to 99 (got '" + id_ + "')");

    FieldRules::requireDisplayName(name_, "category name");

    FieldRules::requireSingleWord(acronym_, "category acronym");
    if (acronym_.size() > maxAcronymLength())
        throw invalid_argument("category acronym must be at most " +
            std::to_string(maxAcronymLength()) +
            " characters (got '" + acronym_ + "')");
}

ProductCategory ProductCategory::withDetails(const string& newName,
    const string& newAcronym) const
{
    return ProductCategory(id_, newName, newAcronym);
}