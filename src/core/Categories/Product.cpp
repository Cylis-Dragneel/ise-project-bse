#include "Product.h"
#include "FieldRules.h"
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
    bool isWhole(double v) { return std::floor(v) == v; }

    void requireAmount(double v, const char* field)
    {
        if (!std::isfinite(v) || v < 0)
            throw invalid_argument(string(field) + " must be a number >= 0");
        if (v > Product::maxValue())
            throw invalid_argument(string(field) + " is too large (maximum " +
                std::to_string(static_cast<long long>(Product::maxValue())) + ")");
    }
}

Product::Product(int id, const string& barcode, const string& categoryId,
    const string& acronym, const string& name, MeasurementUnit unit,
    double unitPrice, double taxAmount, double discountPerUnit,
    double availableQuantity, const string& barcodeImagePath)
    : id_(id), barcode_(barcode), categoryId_(categoryId), acronym_(acronym),
    name_(name), unit_(unit), unitPrice_(unitPrice), taxAmount_(taxAmount),
    discountPerUnit_(discountPerUnit), availableQuantity_(availableQuantity),
    barcodeImagePath_(barcodeImagePath)
{
    if (id_ <= 0)
        throw invalid_argument("product id must be a positive number");

    if (!FieldRules::isDigitsOnly(barcode_) || barcode_.size() != barcodeLength() || barcode_[0] == '0')
        throw invalid_argument("barcode must be exactly " + std::to_string(barcodeLength()) +
            " digits (100000-999999), got '" + barcode_ + "'");

    if (categoryId_ != barcode_.substr(0, categoryIdLength()))
        throw invalid_argument("category id must equal the first " +
            std::to_string(categoryIdLength()) + " digits of the barcode (barcode " +
            barcode_ + " needs category " + barcode_.substr(0, categoryIdLength()) +
            ", got '" + categoryId_ + "')");

    FieldRules::requireSingleWord(acronym_, "acronym");
    if (acronym_.size() > maxAcronymLength())
        throw invalid_argument("acronym must be at most " + std::to_string(maxAcronymLength()) +
            " characters (got '" + acronym_ + "')");

    FieldRules::requireDisplayName(name_, "product name");
    FieldRules::requireSingleWord(barcodeImagePath_, "barcode file path");

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