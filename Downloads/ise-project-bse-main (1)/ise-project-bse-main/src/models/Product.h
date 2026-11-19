#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

struct Product {
    int productId = 0;
    std::string barcode;        // 6-digit, matches decoder output
    std::string acronym;        // max 8 chars
    std::string name;
    std::string unit;           // "count" or "weight"
    double unitPrice = 0;
    double tax = 0;             // PKR per unit
    double discount = 0;        // PKR per unit
    int availableQty = 0;
    std::string barcodeFilePath;

    // Serialize to one line of Product.txt (no trailing '#')
    std::string toLine() const {
        std::ostringstream o;
        o << productId << ' ' << barcode << ' ' << acronym << ' '
          << name << ' ' << unit << ' '
          << std::fixed << std::setprecision(2) << unitPrice << ' '
          << tax << ' ' << discount << ' '
          << availableQty << ' ' << barcodeFilePath << " #";
        return o.str();
    }

       static bool fromLine(const std::string& raw, Product& out) {
        std::string line = raw;
        // Remove the '#' end-of-line marker entirely
        auto hash = line.find('#');
        if (hash != std::string::npos) {
            line = line.substr(0, hash);
        }

        std::istringstream in(line);
        std::string id, price, tax, disc, qty;
        
        // Read exactly the 10 fields
        if (!(in >> id >> out.barcode >> out.acronym
                 >> out.name >> out.unit
                 >> price >> tax >> disc >> qty >> out.barcodeFilePath))
            return false;

        try {
            out.productId    = std::stoi(id);
            out.unitPrice    = std::stod(price);
            out.tax          = std::stod(tax);
            out.discount     = std::stod(disc);
            out.availableQty = std::stoi(qty);
        } catch (...) { return false; }
        return true;
    }
};

#endif