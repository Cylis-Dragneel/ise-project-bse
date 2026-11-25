#include "catalogStockAdjuster.h"

#include <exception>

#include "catalog/ProductCatalog.h"

CatalogStockAdjuster::CatalogStockAdjuster(ProductCatalog &catalog)
    : catalog_(catalog)
{
}

bool CatalogStockAdjuster::adjustStock(const std::vector<StockDelta> &deltas)
{
    std::vector<StockChange> changes;
    changes.reserve(deltas.size());

    for (const StockDelta &delta : deltas)
    {
        std::string barcode = delta.barcode;

        // OrderDetail always carries a barcode, but fall back to the id so a
        // caller that only filled productId still works.
        if (barcode.empty())
        {
            const Product *product = catalog_.findById(delta.productId);
            if (product == nullptr)
            {
                return false;
            }
            barcode = product->barcode();
        }

        StockChange change;
        change.barcode = barcode;
        change.delta = delta.delta;
        changes.push_back(change);
    }

    try
    {
        catalog_.adjustStock(changes);
        return true;
    }
    catch (const std::exception &)
    {
        // adjustStock is all-or-nothing: the catalog is unchanged.
        return false;
    }
}
