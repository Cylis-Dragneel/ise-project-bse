#pragma once
#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>
#include <cstddef>

enum class MeasurementUnit { Count, Weight };

std::string toString(MeasurementUnit unit);
// Case-insensitive: "count" / "weight". Throws std::invalid_argument otherwise.
MeasurementUnit measurementUnitFromString(const std::string& text);

// One product of the store. Validates itself: an invalid Product cannot exist.
class Product
{
private:
    int id_;
    std::string barcode_;
    std::string acronym_;
    std::string name_;
    MeasurementUnit unit_;
    double unitPrice_;
    double taxAmount_;        // PKR
    double discountPerUnit_;  // PKR
    double availableQuantity_;
    std::string barcodeImagePath_;

public:
    static std::size_t maxAcronymLength() { return 8; }

    // Throws std::invalid_argument if any field breaks a rule.
    Product(int id,
        const std::string& barcode,
        const std::string& acronym,
        const std::string& name,
        MeasurementUnit unit,
        double unitPrice,
        double taxAmount,
        double discountPerUnit,
        double availableQuantity,
        const std::string& barcodeImagePath);

    int id() const { return id_; }
    const std::string& barcode() const { return barcode_; }
    const std::string& acronym() const { return acronym_; }
    const std::string& name() const { return name_; }
    MeasurementUnit unit() const { return unit_; }
    double unitPrice() const { return unitPrice_; }
    double taxAmount() const { return taxAmount_; }
    double discountPerUnit() const { return discountPerUnit_; }
    double availableQuantity() const { return availableQuantity_; }
    const std::string& barcodeImagePath() const { return barcodeImagePath_; }

    // True if quantity is valid for this unit (>0, whole number for Count)
    // and does not exceed the available stock.
    bool hasStockFor(double quantity) const;

    // Throws std::out_of_range if the stock is not enough.
    void reduceStock(double quantity);
    void addStock(double quantity);
};

#endif