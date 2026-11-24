#pragma once
#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>
#include <cstddef>

enum class MeasurementUnit { Count, Weight };

std::string toString(MeasurementUnit unit);
MeasurementUnit measurementUnitFromString(const std::string& text);

class Product
{
private:
    int id_;
    std::string barcode_;
    std::string categoryId_;
    std::string acronym_;
    std::string name_;
    MeasurementUnit unit_;
    double unitPrice_;
    double taxAmount_;
    double discountPerUnit_;
    double availableQuantity_;
    std::string barcodeImagePath_;

public:
    static std::size_t maxAcronymLength() { return 8; }
    static std::size_t barcodeLength() { return 6; }
    static std::size_t categoryIdLength() { return 2; }
    static double maxValue() { return 1000000000.0; }

    Product(int id,
        const std::string& barcode,
        const std::string& categoryId,
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
    const std::string& categoryId() const { return categoryId_; }
    const std::string& acronym() const { return acronym_; }
    const std::string& name() const { return name_; }
    MeasurementUnit unit() const { return unit_; }
    double unitPrice() const { return unitPrice_; }
    double taxAmount() const { return taxAmount_; }
    double discountPerUnit() const { return discountPerUnit_; }
    double availableQuantity() const { return availableQuantity_; }
    const std::string& barcodeImagePath() const { return barcodeImagePath_; }

    bool hasStockFor(double quantity) const;
    void reduceStock(double quantity);
    void addStock(double quantity);
};

#endif