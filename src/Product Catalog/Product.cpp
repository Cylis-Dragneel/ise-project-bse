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

    bool isDigitsOnly(const string& s)
    {
        if (s.empty()) return false;
        for (size_t i = 0; i < s.size(); i++)
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        return true;
    }

    bool isWhole(double v) { return std::floor(v) == v; }

    void requireNonNegativeNumber(double v, const char* field)
    {
        if (!std::isfinite(v) || v < 0)
            throw invalid_argument(string(field) + " must be a number >= 0");
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
    if (!isDigitsOnly(barcode_))
        throw invalid_argument("barcode must contain digits only (got '" + barcode_ + "')");
    if (acronym_.empty() || acronym_.size() > maxAcronymLength() || hasWhitespace(acronym_))
        throw invalid_argument("acronym must be 1-" + std::to_string(maxAcronymLength()) +
            " characters without spaces (got '" + acronym_ + "')");
    if (name_.empty())
        throw invalid_argument("product name must not be empty");
    if (barcodeImagePath_.empty() || hasWhitespace(barcodeImagePath_))
        throw invalid_argument("barcode file path must be non-empty and contain no spaces");

    requireNonNegativeNumber(unitPrice_, "unit price");
    requireNonNegativeNumber(taxAmount_, "tax");
    requireNonNegativeNumber(discountPerUnit_, "discount");
    requireNonNegativeNumber(availableQuantity_, "available quantity");

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
    availableQuantity_ += quantity;
}