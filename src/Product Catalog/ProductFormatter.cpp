#include "ProductFormatter.h"
#include "NumberFormat.h"

std::string ProductFormatter::describe(const Product& p)
{
    std::string unitLabel = (p.unit() == MeasurementUnit::Count) ? "per item" : "per kg";

    return "Product ID    : " + std::to_string(p.id()) + "\n" +
        "Barcode       : " + p.barcode() + "\n" +
        "Name          : " + p.name() + " (" + p.acronym() + ")\n" +
        "Sold by       : " + toString(p.unit()) + "\n" +
        "Unit price    : PKR " + NumberFormat::money(p.unitPrice()) + " " + unitLabel + "\n" +
        "Tax           : PKR " + NumberFormat::money(p.taxAmount()) + "\n" +
        "Discount      : PKR " + NumberFormat::money(p.discountPerUnit()) + " per unit\n" +
        "In stock      : " + NumberFormat::general(p.availableQuantity()) + "\n" +
        "Barcode image : " + p.barcodeImagePath() + "\n";
}