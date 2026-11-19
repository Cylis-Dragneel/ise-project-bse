#include "ProductCatalog.h"
#include <stdexcept>

using std::string;
using std::vector;

ProductCatalog::ProductCatalog(std::shared_ptr<IProductRepository> repository)
    : repository_(std::move(repository))
{
    if (!repository_)
        throw std::invalid_argument("ProductCatalog needs a repository");
}

void ProductCatalog::load()
{
    vector<Product> loaded = repository_->loadAll();

    std::unordered_map<string, size_t> byBarcode;
    std::map<int, size_t> byId;
    for (size_t i = 0; i < loaded.size(); i++)
    {
        if (!byId.insert(std::make_pair(loaded[i].id(), i)).second)
            throw std::invalid_argument("duplicate product id: " + std::to_string(loaded[i].id()));
        if (!byBarcode.insert(std::make_pair(loaded[i].barcode(), i)).second)
            throw std::invalid_argument("duplicate barcode: " + loaded[i].barcode());
    }

    // Only replace the current data once everything is known to be valid.
    products_.swap(loaded);
    indexByBarcode_.swap(byBarcode);
    indexById_.swap(byId);
}

const Product* ProductCatalog::findByBarcode(const string& barcode) const
{
    auto it = indexByBarcode_.find(barcode);
    return it == indexByBarcode_.end() ? nullptr : &products_[it->second];
}

const Product* ProductCatalog::findByBarcode(int barcode) const
{
    return findByBarcode(std::to_string(barcode));
}

const Product* ProductCatalog::findById(int id) const
{
    auto it = indexById_.find(id);
    return it == indexById_.end() ? nullptr : &products_[it->second];
}

bool ProductCatalog::isAvailable(const string& barcode, double quantity) const
{
    const Product* p = findByBarcode(barcode);
    return p != nullptr && p->hasStockFor(quantity);
}

void ProductCatalog::deductStock(const vector<StockRequest>& requests)
{
    if (requests.empty()) return; // nothing to change, nothing to save

    // Same product may appear on several lines: add the quantities up first.
    std::map<string, double> totals;
    for (size_t i = 0; i < requests.size(); i++)
        totals[requests[i].barcode] += requests[i].quantity;

    // 1) validate everything, change nothing
    for (auto it = totals.begin(); it != totals.end(); ++it)
    {
        const Product* p = findByBarcode(it->first);
        if (p == nullptr)
            throw std::invalid_argument("unknown product barcode: " + it->first);
        if (!p->hasStockFor(it->second))
            throw std::out_of_range("not enough stock of '" + p->acronym() + "'");
    }

    // 2) apply, keeping a backup so a failed save can be undone
    const vector<Product> backup = products_;
    for (auto it = totals.begin(); it != totals.end(); ++it)
        products_[indexByBarcode_[it->first]].reduceStock(it->second);

    try
    {
        repository_->saveAll(products_);
    }
    catch (...)
    {
        products_ = backup;
        throw;
    }
}