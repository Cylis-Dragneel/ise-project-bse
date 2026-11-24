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

    void rebuildIndexes();
    void rejectDuplicateAcronym(const Product& product, std::size_t ignoreIndex) const;

public:
    explicit ProductCatalog(std::shared_ptr<IProductRepository> repository);

    // (Re)loads everything from the repository. Throws on unreadable data or
    // duplicate product ids / barcodes / acronyms. Pointers from earlier
    // find...() calls become invalid after load().
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

    // Manager operations. All persist through the repository and roll the
    // in-memory list back when saving fails.
    // Throws std::invalid_argument for a duplicate id / barcode / acronym.
    void addProduct(const Product& product);
    // Replaces the product that has the same id. The barcode and category are
    // expected to stay the same (history refers to them); the model itself does
    // not forbid a change.
    void adjustStock(const std::vector<StockChange>& changes);
    void updateProduct(const Product& product);
    // Direct stock correction used by the inventory screen.
    void setStock(const std::string& barcode, double quantity);
};

#endif