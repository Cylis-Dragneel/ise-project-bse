#ifndef PRODUCTMANAGER_H
#define PRODUCTMANAGER_H

#include <string>
#include <vector>
#include "../models/Product.h"

class ProductManager {
public:
    explicit ProductManager(const std::string& path = "Product.txt");

    // Reload from disk. Returns false if file missing.
    bool load();

    // Find by barcode string (exact match). Returns false if not found.
    bool findByBarcode(const std::string& barcode, Product& out) const;

    // Find by product id.
    bool findById(int id, Product& out) const;

    const std::vector<Product>& all() const { return products; }

    // Decrement available qty in memory and rewrite Product.txt.
    // Returns false if insufficient stock or write fails.
    bool decrementQty(int productId, int qty);

private:
    std::string filePath;
    std::vector<Product> products;
    bool saveAll() const;
};

#endif