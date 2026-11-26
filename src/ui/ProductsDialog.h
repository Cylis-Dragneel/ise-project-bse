#ifndef PRODUCTS_DIALOG_H
#define PRODUCTS_DIALOG_H

#include <QDialog>

class ProductCatalog;
class CategoryCatalog;
class QTableWidget;

// Manager dialog combining Products and Categories (requirements 1 and 6).
class ProductsDialog : public QDialog
{
    Q_OBJECT
public:
    ProductsDialog(ProductCatalog *catalog,
                   CategoryCatalog *categories,
                   QWidget *parent = nullptr);

private:
    ProductCatalog *catalog_;
    CategoryCatalog *categories_;
    QTableWidget *productTable_ = nullptr;
    QTableWidget *categoryTable_ = nullptr;

    void refreshProducts();
    void refreshCategories();
    void addProduct();
    void editProduct(int row);
    void addCategory();
    void editCategory(int row);
};

#endif // PRODUCTS_DIALOG_H