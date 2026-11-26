#ifndef INVENTORY_DIALOG_H
#define INVENTORY_DIALOG_H

#include <QDialog>

class ProductCatalog;
class QTableWidget;

// Manager inventory editor (requirement 4): shows all products + stock,
// lets the manager change a quantity and persists via ProductCatalog::setStock.
class InventoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InventoryDialog(ProductCatalog *catalog, QWidget *parent = nullptr);

private:
    ProductCatalog *catalog_;
    QTableWidget *table_ = nullptr;

    void refresh();
    void applyEdit(int row);
};

#endif