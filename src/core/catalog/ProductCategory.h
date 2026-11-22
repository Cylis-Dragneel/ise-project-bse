#ifndef PRODUCT_CATEGORY_H
#define PRODUCT_CATEGORY_H

#include <cstddef>
#include <string>

// One product category of PCategory.txt:
//   categoryId name acronym #
//
// The category id is two digits and becomes the first two digits of every
// barcode generated for a product of this category. The range is 10-99 because
// the barcode encoder only accepts 6-digit barcodes 100000-999999 (no leading
// zero).
class ProductCategory
{
private:
    std::string id_;
    std::string name_;
    std::string acronym_;

public:
    static std::size_t idLength() { return 2; }
    static std::size_t maxAcronymLength() { return 8; }

    // Throws std::invalid_argument if any field breaks a rule.
    ProductCategory(const std::string& id,
                    const std::string& name,
                    const std::string& acronym);

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& acronym() const { return acronym_; }
};

#endif // PRODUCT_CATEGORY_H
