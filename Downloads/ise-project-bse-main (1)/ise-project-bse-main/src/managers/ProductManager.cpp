#include "ProductManager.h"
#include <fstream>
#include <algorithm>

ProductManager::ProductManager(const std::string& path) 
    : filePath("Product.txt") {
    load();
}

bool ProductManager::load() {
    products.clear();
    std::ifstream f(filePath);
    if (!f.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        Product p;
        if (Product::fromLine(line, p)) {
            products.push_back(p);
        }
    }
    return true;
}

bool ProductManager::findByBarcode(const std::string& barcode, Product& out) const {
    std::string cleanBarcode = barcode;
    cleanBarcode.erase(std::remove_if(cleanBarcode.begin(), cleanBarcode.end(), ::isspace), cleanBarcode.end());

    for (const auto& p : products) {
        if (p.barcode == cleanBarcode) { 
            out = p; 
            return true; 
        }
    }
    return false;
}

bool ProductManager::findById(int id, Product& out) const {
    for (const auto& p : products)
        if (p.productId == id) { out = p; return true; }
    return false;
}

bool ProductManager::saveAll() const {
    std::ofstream f(filePath, std::ios::trunc);
    if (!f.is_open()) return false;
    for (const auto& p : products) f << p.toLine() << '\n';
    return f.good();
}

bool ProductManager::decrementQty(int productId, int qty) {
    for (auto& p : products) {
        if (p.productId == productId) {
            if (p.availableQty < qty) return false;
            p.availableQty -= qty;
            return saveAll();
        }
    }
    return false;
}