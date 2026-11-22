#include "ProductCatalog.h"
#include <cctype>
#include <cmath>
#include <stdexcept>

using std::string;
using std::vector;

namespace
{
    string lowerCopy(const string& text)
    {
        string lower;
        lower.reserve(text.size());
        for (size_t i = 0; i < text.size(); i++)
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
        return lower;
    }
}

ProductCatalog::ProductCatalog(std::shared_ptr<IProductRepository> repository)
    : repository_(std::move(repository))
{
    if (!repository_)
        throw std::invalid_argument("ProductCatalog needs a repository");
}

void ProductCatalog::rebuildIndexes()
{
    indexByBarcode_.clear();
    indexById_.clear();
    for (size_t i = 0; i < products_.size(); i++)
    {
        indexByBarcode_[products_[i].barcode()] = i;
        indexById_[products_[i].id()] = i;
    }
}

void ProductCatalog::rejectDuplicateAcronym(const Product& product,
                                            std::size_t ignoreIndex) const
{
    const string wanted = lowerCopy(product.acronym());
    for (size_t i = 0; i < products_.size(); i++)
    {
        if (i == ignoreIndex) continue;
        if (lowerCopy(products_[i].acronym()) == wanted)
            throw std::invalid_argument("duplicate product acronym: " + product.acronym());
    }
}

void ProductCatalog::load()
{
    vector<Product> loaded = repository_->loadAll();

    std::unordered_map<string, size_t> byBarcode;
    std::map<int, size_t> byId;
    std::map<string, size_t> byAcronym;
    for (size_t i = 0; i < loaded.size(); i++)
    {
        if (!byId.insert(std::make_pair(loaded[i].id(), i)).second)
            throw std::invalid_argument("duplicate product id: " + std::to_string(loaded[i].id()));
        if (!byBarcode.insert(std::make_pair(loaded[i].barcode(), i)).second)
            throw std::invalid_argument("duplicate barcode: " + loaded[i].barcode());
        if (!byAcronym.insert(std::make_pair(lowerCopy(loaded[i].acronym()), i)).second)
            throw std::invalid_argument("duplicate product acronym: " + loaded[i].acronym());
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
        if (!std::isfinite(it->second) || it->second <= 0)
            throw std::invalid_argument(
                "quantity must be a positive number for barcode " + it->first);

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
        rebuildIndexes();
        throw;
    }
}

void ProductCatalog::addProduct(const Product& product)
{
    if (indexById_.find(product.id()) != indexById_.end())
        throw std::invalid_argument("duplicate product id: " + std::to_string(product.id()));
    if (indexByBarcode_.find(product.barcode()) != indexByBarcode_.end())
        throw std::invalid_argument("duplicate barcode: " + product.barcode());
    rejectDuplicateAcronym(product, products_.size());

    const vector<Product> backup = products_;
    products_.push_back(product);
    rebuildIndexes();

    try
    {
        repository_->saveAll(products_);
    }
    catch (...)
    {
        products_ = backup;
        rebuildIndexes();
        throw;
    }
}

void ProductCatalog::updateProduct(const Product& product)
{
    auto found = indexById_.find(product.id());
    if (found == indexById_.end())
        throw std::invalid_argument("unknown product id: " + std::to_string(product.id()));

    // Another product may not already use this barcode or acronym.
    auto barcodeOwner = indexByBarcode_.find(product.barcode());
    if (barcodeOwner != indexByBarcode_.end() && barcodeOwner->second != found->second)
        throw std::invalid_argument("duplicate barcode: " + product.barcode());
    rejectDuplicateAcronym(product, found->second);

    const vector<Product> backup = products_;
    products_[found->second] = product;
    rebuildIndexes();

    try
    {
        repository_->saveAll(products_);
    }
    catch (...)
    {
        products_ = backup;
        rebuildIndexes();
        throw;
    }
}

void ProductCatalog::setStock(const string& barcode, double quantity)
{
    auto found = indexByBarcode_.find(barcode);
    if (found == indexByBarcode_.end())
        throw std::invalid_argument("unknown product barcode: " + barcode);

    const vector<Product> backup = products_;
    products_[found->second].setAvailableQuantity(quantity);

    try
    {
        repository_->saveAll(products_);
    }
    catch (...)
    {
        products_ = backup;
        rebuildIndexes();
        throw;
    }
}