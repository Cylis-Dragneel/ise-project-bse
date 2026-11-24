#ifndef PRODUCTCATALOG_H
#define PRODUCTCATALOG_H

#include "IProductRepository.h"
#include "Product.h"
#include "StockChange.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

class ProductCatalog {
public:
    explicit ProductCatalog(std::shared_ptr<IProductRepository> repository);

    void load();
    void saveAll();

    const Product* findById(int id) const;
    const Product* findByBarcode(const std::string& barcode) const;
    const Product* findByAcronym(const std::string& acronym) const;
    std::vector<Product> all() const;

    void addProduct(const Product& product);
    void updateProduct(const Product& edited);
    void setStock(const std::string& barcode, double quantity);
    void adjustStock(const std::vector<StockChange>& changes);
    void deductStock(const std::vector<StockChange>& changes);
    int  nextProductId() const;

private:
    void rebuildIndexes();
    void commit(const std::vector<Product>& next);

    std::shared_ptr<IProductRepository> repository_;
    std::vector<Product> products_;
    std::map<int, std::size_t> byId_;
    std::map<std::string, std::size_t> byBarcode_;
    std::map<std::string, std::size_t> byAcronym_;
};

#endif