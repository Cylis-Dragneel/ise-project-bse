#ifndef PRODUCT_EDITOR_DIALOG_H
#define PRODUCT_EDITOR_DIALOG_H

#include <QDialog>
#include <string>

class CategoryCatalog;
class ProductCatalog;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;

// Add/edit a product (requirement 6) + generate its barcode (requirement 1).
class ProductEditorDialog : public QDialog
{
    Q_OBJECT
public:
    // Pass an existing product to edit, or nullptr to add a new one.
    ProductEditorDialog(ProductCatalog *catalog,
                        CategoryCatalog *categories,
                        int existingProductId,
                        QWidget *parent = nullptr);

private:
    ProductCatalog *catalog_;
    CategoryCatalog *categories_;
    int existingId_;
    std::string generatedBarcode_;

    QLineEdit *nameEdit_ = nullptr;
    QLineEdit *acronymEdit_ = nullptr;
    QComboBox *categoryCombo_ = nullptr;
    QComboBox *unitCombo_ = nullptr;
    QDoubleSpinBox *priceSpin_ = nullptr;
    QDoubleSpinBox *taxSpin_ = nullptr;
    QDoubleSpinBox *discountSpin_ = nullptr;
    QDoubleSpinBox *quantitySpin_ = nullptr;
    QLineEdit *barcodeEdit_ = nullptr;
    QPushButton *generateButton_ = nullptr;

    void buildUi();
    void populateFromExisting();
    void generateBarcode();
    void onSave();
    bool persist(const std::string &barcode);
};

#endif