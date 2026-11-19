#include "ProductResolver.h"

#include "NumberFormat.h"
#include "ProductCatalog.h"
#include "ProductFormatter.h"

#include <cctype>

namespace {

std::string trim(const std::string &text)
{
    std::size_t begin = 0;
    std::size_t end = text.size();

    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])))
        ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
        --end;

    return text.substr(begin, end - begin);
}

// Product barcodes are the same 6-digit values the encoder produces.
bool isProductBarcode(const std::string &barcode)
{
    if (barcode.size() != Product::barcodeLength() || barcode[0] == '0')
        return false;

    for (char character : barcode)
    {
        if (std::isdigit(static_cast<unsigned char>(character)) == 0)
            return false;
    }

    return true;
}

std::string stockMessage(const Product &product, double requested)
{
    return "Not enough stock: " + NumberFormat::general(requested) +
           " requested, " + NumberFormat::general(product.availableQuantity()) +
           " available (" + product.acronym() + ").";
}

} // namespace

ProductResolver::ProductResolver(const ProductCatalog &catalog)
    : catalog_(catalog)
{
}

ProductResolver::Result ProductResolver::lookup(const std::string &barcodeIn) const
{
    Result result;
    const std::string barcode = trim(barcodeIn);

    if (!isProductBarcode(barcode))
    {
        result.status = Status::InvalidBarcode;
        result.message = barcode.empty()
                             ? "Scan or enter a 6-digit barcode."
                             : "Barcode must be 6 digits from 100000 to 999999 "
                               "(got '" + barcode + "').";
        return result;
    }

    const Product *product = catalog_.findByBarcode(barcode);
    if (product == nullptr)
    {
        result.status = Status::UnknownBarcode;
        result.message = "No product with barcode " + barcode + ".";
        return result;
    }

    result.status = Status::Found;
    result.product = product;
    result.description = ProductFormatter::describe(*product);
    return result;
}

ProductResolver::Result ProductResolver::lookupForQuantity(
    const std::string &barcode, double quantity) const
{
    Result result = lookup(barcode);
    if (!result.found())
        return result;

    // Also rejects NaN.
    if (!(quantity > 0))
    {
        result.status = Status::InsufficientStock;
        result.message = "Quantity must be greater than 0.";
        return result;
    }

    if (!result.product->hasStockFor(quantity))
    {
        result.status = Status::InsufficientStock;
        result.message = stockMessage(*result.product, quantity);
    }

    return result;
}
