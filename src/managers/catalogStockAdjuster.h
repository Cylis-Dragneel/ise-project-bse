#ifndef CATALOG_STOCK_ADJUSTER_H
#define CATALOG_STOCK_ADJUSTER_H

#include <vector>

#include "orderService.h"

class ProductCatalog;

// Connects Person B's OrderService to the real ProductCatalog.
//
// OrderService speaks StockDelta{productId, barcode, delta}; the catalog speaks
// StockChange{barcode, delta}. This adapter converts one to the other and turns
// the catalog's exceptions into the boolean contract IStockAdjuster expects:
// false means nothing changed.
class CatalogStockAdjuster : public IStockAdjuster
{
public:
    explicit CatalogStockAdjuster(ProductCatalog &catalog);

    bool adjustStock(const std::vector<StockDelta> &deltas) override;

private:
    ProductCatalog &catalog_;
};

#endif // CATALOG_STOCK_ADJUSTER_H
