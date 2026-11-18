#pragma once
#ifndef PRODUCT_CATALOG_H
#define PRODUCT_CATALOG_H

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "IProductRepository.h"
#include "Product.h"

// A request to take 'quantity' of the product with this barcode out of stock.
struct StockRequest
{
    std::string barcode;
    double quantity;
};

// The business view of the products: look up by barcode / id, check stock,
// deduct stock when a bill is generated. Knows nothing about files or GUI.
class ProductCatalog
{
private:
    std::shared_ptr<IProductRepository> repository_;
    std::vector<Product> products_;
    std::unordered_map<std::string, size_t> indexByBarcode_;
    std::map<int, size_t> indexById_;

public:
    explicit ProductCatalog(std::shared_ptr<IProductRepository> repository);

    // (Re)loads everything from the repository. Throws on unreadable data or
    // duplicate product ids / barcodes. Pointers from earlier find...() calls
    // become invalid after load().
    void load();

    // nullptr when the product does not exist. The pointer stays valid until
    // the next load().
    const Product* findByBarcode(const std::string& barcode) const;
    const Product* findByBarcode(int barcode) const; // e.g. result of BarcodeDecoder::decode
    const Product* findById(int id) const;

    const std::vector<Product>& all() const { return products_; }

    // Unknown barcode -> false.
    bool isAvailable(const std::string& barcode, double quantity) const;

    // All-or-nothing: if any line is unknown or short of stock, NOTHING is
    // changed (std::invalid_argument / std::out_of_range). On success the
    // new stock is saved through the repository; if saving fails the in-memory
    // stock is rolled back and the exception is rethrown.
    void deductStock(const std::vector<StockRequest>& requests);
};

#endif