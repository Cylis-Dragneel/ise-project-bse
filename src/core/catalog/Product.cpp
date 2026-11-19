#include "Product.h"
#include <cctype>
#include <cmath>
#include <stdexcept>

using std::invalid_argument;
using std::string;

string toString(MeasurementUnit unit)
{
    return unit == MeasurementUnit::Count ? "count" : "weight";
}

MeasurementUnit measurementUnitFromString(const string& text)
{
    string lower;
    for (size_t i = 0; i < text.size(); i++)
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));

    if (lower == "count")  return MeasurementUnit::Count;
    if (lower == "weight") return MeasurementUnit::Weight;
    throw invalid_argument("measurement unit must be 'count' or 'weight' (got '" + text + "')");
}

namespace
{
    bool hasWhitespace(const string& s)
    {
        for (size_t i = 0; i < s.size(); i++)
            if (std::isspace(static_cast<unsigned char>(s[i]))) return true;
        return false;
    }

    // Control characters (newline, tab, NUL, ...). Bytes >= 0x80 are allowed so
    // UTF-8 names (e.g. Urdu) still work.
    bool hasControlChar(const string& s)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (c < 0x20 || c == 0x7F) return true;
        }
        return false;
    }

    bool contains(const string& s, char c) { return s.find(c) != string::npos; }

    bool isDigitsOnly(const string& s)
    {
        if (s.empty()) return false;
        for (size_t i = 0; i < s.size(); i++)
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        return true;
    }

    bool isWhole(double v) { return std::floor(v) == v; }

    void requireAmount(double v, const char* field)
    {
        if (!std::isfinite(v) || v < 0)
            throw invalid_argument(string(field) + " must be a number >= 0");
        if (v > Product::maxValue())
            throw invalid_argument(string(field) + " is too large (maximum " +
                std::to_string(static_cast<long long>(Product::maxValue())) + ")");
    }

    // Acronym and file path are single words inside the record.
    void requireSingleWord(const string& value, const char* field)
    {
        if (value.empty())
            throw invalid_argument(string(field) + " must not be empty");
        if (hasWhitespace(value))
            throw invalid_argument(string(field) + " must not contain spaces (got '" + value + "')");
        if (hasControlChar(value))
            throw invalid_argument(string(field) + " must not contain control characters");
        if (contains(value, '#'))
            throw invalid_argument(string(field) + " must not contain '#' (it ends a record in Product.txt)");
    }

    // A name may contain spaces, but must survive a write/read round trip:
    // the file stores spaces as '_', so '_' itself cannot be part of a name.
    void requireValidName(const string& name)
    {
        if (name.empty())
            throw invalid_argument("product name must not be empty");
        if (hasControlChar(name))
            throw invalid_argument("product name must not contain control characters (e.g. new lines)");
        if (contains(name, '#'))
            throw invalid_argument("product name must not contain '#' (it ends a record in Product.txt)");
        if (contains(name, '_'))
            throw invalid_argument("product name must not contain '_' (it stands for a space in Product.txt)");
        if (name.front() == ' ' || name.back() == ' ')
            throw invalid_argument("product name must not start or end with a space");
        if (name.find("  ") != string::npos)
            throw invalid_argument("product name must not contain two spaces in a row");
    }
}

Product::Product(int id, const string& barcode, const string& acronym,
    const string& name, MeasurementUnit unit, double unitPrice,
    double taxAmount, double discountPerUnit,
    double availableQuantity, const string& barcodeImagePath)
    : id_(id), barcode_(barcode), acronym_(acronym), name_(name), unit_(unit),
    unitPrice_(unitPrice), taxAmount_(taxAmount),
    discountPerUnit_(discountPerUnit), availableQuantity_(availableQuantity),
    barcodeImagePath_(barcodeImagePath)
{
    if (id_ <= 0)
        throw invalid_argument("product id must be a positive number");

    if (!isDigitsOnly(barcode_) || barcode_.size() != barcodeLength() || barcode_[0] == '0')
        throw invalid_argument("barcode must be exactly " + std::to_string(barcodeLength()) +
            " digits (100000-999999), got '" + barcode_ + "'");

    requireSingleWord(acronym_, "acronym");
    if (acronym_.size() > maxAcronymLength())
        throw invalid_argument("acronym must be at most " + std::to_string(maxAcronymLength()) +
            " characters (got '" + acronym_ + "')");

    requireValidName(name_);
    requireSingleWord(barcodeImagePath_, "barcode file path");

    requireAmount(unitPrice_, "unit price");
    requireAmount(taxAmount_, "tax");
    requireAmount(discountPerUnit_, "discount");
    requireAmount(availableQuantity_, "available quantity");

    if (discountPerUnit_ > unitPrice_)
        throw invalid_argument("discount cannot be greater than the unit price");
    if (unit_ == MeasurementUnit::Count && !isWhole(availableQuantity_))
        throw invalid_argument("available quantity of a 'count' product must be a whole number");
}

bool Product::hasStockFor(double quantity) const
{
    if (!std::isfinite(quantity) || quantity <= 0) return false;
    if (unit_ == MeasurementUnit::Count && !isWhole(quantity)) return false;
    return quantity <= availableQuantity_;
}

void Product::reduceStock(double quantity)
{
    if (!hasStockFor(quantity))
        throw std::out_of_range("not enough stock of '" + acronym_ + "' for the requested quantity");
    availableQuantity_ -= quantity;
}

void Product::addStock(double quantity)
{
    if (!std::isfinite(quantity) || quantity <= 0)
        throw invalid_argument("quantity to add must be > 0");
    if (unit_ == MeasurementUnit::Count && !isWhole(quantity))
        throw invalid_argument("quantity of a 'count' product must be a whole number");
    if (availableQuantity_ + quantity > maxValue())
        throw invalid_argument("stock would exceed the maximum allowed quantity");
    availableQuantity_ += quantity;
}