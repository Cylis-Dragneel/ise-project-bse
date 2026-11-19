#ifndef PRODUCT_RESOLVER_H
#define PRODUCT_RESOLVER_H

#include <string>
#include "Product.h"

class ProductCatalog;

// Turns a scanned barcode into a product and the text the UI shows.
//
// This is the single place where "scan -> find -> describe" happens, so the
// cashier order screen and the manager description view behave identically.
// Qt-free, so it can be tested without a camera or a window.
class ProductResolver
{
public:
    enum class Status
    {
        Found,
        InvalidBarcode,
        UnknownBarcode,
        InsufficientStock
    };

    struct Result
    {
        Status status = Status::InvalidBarcode;

        // Set when the barcode resolved to a product, including when only the
        // quantity was rejected, so the UI can still show the specifications.
        const Product *product = nullptr;

        // User-facing explanation. Empty only when status is Found.
        std::string message;

        // ProductFormatter::describe() output. Empty unless product is set.
        std::string description;

        bool found() const { return status == Status::Found; }
    };

    explicit ProductResolver(const ProductCatalog &catalog);

    // Leading/trailing whitespace is ignored. Never throws.
    Result lookup(const std::string &barcode) const;

    // lookup() plus the stock check. On InsufficientStock the product and
    // description stay filled in; only the quantity is refused.
    Result lookupForQuantity(const std::string &barcode, double quantity) const;

private:
    const ProductCatalog &catalog_;
};

#endif // PRODUCT_RESOLVER_H
