#include "ProductCatalog.h"
#include "FieldRules.h"
#include "StockChange.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

ProductCatalog::ProductCatalog(std::shared_ptr<IProductRepository> repository)
    : repository_(std::move(repository)) {
    if (!repository_) {
        throw std::invalid_argument("product catalog requires a repository");
    }
}

void ProductCatalog::load() {
    products_ = repository_->loadAll();
    rebuildIndexes();
    for (std::size_t i = 0; i < products_.size(); ++i) {
        for (std::size_t j = i + 1; j < products_.size(); ++j) {
            if (FieldRules::equalsIgnoreCase(products_[i].acronym(),
                products_[j].acronym())) {
                throw std::invalid_argument(
                    "duplicate product acronym: " + products_[i].acronym());
            }
        }
    }
}

static std::string makeLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

void ProductCatalog::rebuildIndexes() {
    byId_.clear();
    byBarcode_.clear();
    byAcronym_.clear();
    for (std::size_t i = 0; i < products_.size(); ++i) {
        byId_[products_[i].id()] = i;
        byBarcode_[products_[i].barcode()] = i;
        byAcronym_[makeLower(products_[i].acronym())] = i;
    }
}

void ProductCatalog::saveAll() {
    repository_->saveAll(products_);
}

void ProductCatalog::commit(const std::vector<Product>& next) {
    repository_->saveAll(next);
    products_ = next;
    rebuildIndexes();
}

const Product* ProductCatalog::findById(int id) const {
    auto it = byId_.find(id);
    return it == byId_.end() ? nullptr : &products_[it->second];
}

const Product* ProductCatalog::findByBarcode(const std::string& barcode) const {
    auto it = byBarcode_.find(barcode);
    return it == byBarcode_.end() ? nullptr : &products_[it->second];
}

const Product* ProductCatalog::findByAcronym(const std::string& acronym) const {
    auto it = byAcronym_.find(makeLower(acronym));
    return it == byAcronym_.end() ? nullptr : &products_[it->second];
}

std::vector<Product> ProductCatalog::all() const {
    return products_;
}

// -----------------------------------------------------------------------------
// Module 7: Stock Operations
// -----------------------------------------------------------------------------

void ProductCatalog::deductStock(const std::vector<StockChange>& changes) {
    adjustStock(changes);
}

void ProductCatalog::setStock(const std::string& barcode, double quantity) {
    if (quantity < 0.0) {
        throw std::invalid_argument("stock quantity must not be negative");
    }
    auto it = byBarcode_.find(barcode);
    if (it == byBarcode_.end()) {
        throw std::out_of_range("unknown product barcode: " + barcode);
    }

    std::vector<Product> next = products_;
    const Product& old = products_[it->second];

    // Create the updated product using YOUR EXACT getter names
    next[it->second] = Product(old.id(), old.barcode(), old.categoryId(),
        old.acronym(), old.name(), old.unit(),
        old.unitPrice(), old.taxAmount(), old.discountPerUnit(),
        quantity, old.barcodeImagePath());
    commit(next);
}

void ProductCatalog::adjustStock(const std::vector<StockChange>& changes) {
    if (changes.empty()) {
        return;
    }
    std::vector<Product> next = products_;

    for (const StockChange& change : changes) {
        auto it = byBarcode_.find(change.barcode);
        if (it == byBarcode_.end()) {
            throw std::out_of_range("unknown product barcode: " + change.barcode);
        }

        Product current = next[it->second];

        // Use your exact built-in methods!
        if (change.delta > 0) {
            current.addStock(change.delta);
        }
        else if (change.delta < 0) {
            current.reduceStock(-change.delta);
        }

        next[it->second] = current;
    }
    commit(next);
}

int ProductCatalog::nextProductId() const {
    int maxId = 0;
    for (const Product& p : products_) {
        if (p.id() > maxId) maxId = p.id();
    }
    return maxId + 1;
}

// -----------------------------------------------------------------------------
// Module 8: Product Add / Edit
// -----------------------------------------------------------------------------

void ProductCatalog::addProduct(const Product& product) {
    if (byId_.count(product.id()) != 0) {
        throw std::invalid_argument("duplicate product id: " +
            std::to_string(product.id()));
    }
    if (byBarcode_.count(product.barcode()) != 0) {
        throw std::invalid_argument("duplicate product barcode: " + product.barcode());
    }
    if (byAcronym_.count(makeLower(product.acronym())) != 0) {
        throw std::invalid_argument("duplicate product acronym: " + product.acronym());
    }
    std::vector<Product> next = products_;
    next.push_back(product);
    commit(next);
}

void ProductCatalog::updateProduct(const Product& edited) {
    auto it = byId_.find(edited.id());
    if (it == byId_.end()) {
        throw std::out_of_range("unknown product id: " + std::to_string(edited.id()));
    }
    const Product& current = products_[it->second];
    if (current.barcode() != edited.barcode()) {
        throw std::invalid_argument("product barcode is immutable");
    }
    if (current.categoryId() != edited.categoryId()) {
        throw std::invalid_argument("product category is immutable");
    }
    if (!FieldRules::equalsIgnoreCase(current.acronym(), edited.acronym())) {
        throw std::invalid_argument("product acronym is immutable");
    }
    if (current.unit() != edited.unit()) {
        throw std::invalid_argument("product unit is immutable");
    }
    if (edited.acronym().empty()) {
        throw std::invalid_argument("product acronym must not be empty");
    }
    std::vector<Product> next = products_;
    next[it->second] = edited;
    commit(next);
}