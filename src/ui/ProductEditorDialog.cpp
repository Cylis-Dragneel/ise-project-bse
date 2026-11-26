#include "ProductEditorDialog.h"
#include "barcodeEncoder.h"

#include "catalog/CategoryCatalog.h"
#include "catalog/ProductCatalog.h"
#include "catalog/ProductCategory.h"
#include "catalog/BarcodeAllocator.h"
#include "renderer/BarcodeWidget.h"

#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <exception>
#include <vector>

ProductEditorDialog::ProductEditorDialog(ProductCatalog *catalog,
                                         CategoryCatalog *categories,
                                         int existingProductId,
                                         QWidget *parent)
    : QDialog(parent),
      catalog_(catalog),
      categories_(categories),
      existingId_(existingProductId)
{
    setWindowTitle(existingProductId > 0 ? "Edit Product" : "Add Product");
    resize(480, 480);
    buildUi();
    if (existingId_ > 0)
    {
        populateFromExisting();
    }
}

void ProductEditorDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    nameEdit_ = new QLineEdit(this);
    acronymEdit_ = new QLineEdit(this);
    acronymEdit_->setMaxLength(8);

    categoryCombo_ = new QComboBox(this);
    for (const ProductCategory &c : categories_->all())
    {
        categoryCombo_->addItem(
            QString("%1 - %2").arg(QString::fromStdString(c.id()),
                                   QString::fromStdString(c.name())),
            QString::fromStdString(c.id()));
    }

    unitCombo_ = new QComboBox(this);
    unitCombo_->addItem("Count", "count");
    unitCombo_->addItem("Weight", "weight");

    priceSpin_ = new QDoubleSpinBox(this);
    priceSpin_->setRange(0.0, 1000000000.0);
    priceSpin_->setDecimals(2);

    taxSpin_ = new QDoubleSpinBox(this);
    taxSpin_->setRange(0.0, 1000000000.0);
    taxSpin_->setDecimals(2);

    discountSpin_ = new QDoubleSpinBox(this);
    discountSpin_->setRange(0.0, 1000000000.0);
    discountSpin_->setDecimals(2);

    quantitySpin_ = new QDoubleSpinBox(this);
    quantitySpin_->setRange(0.0, 1000000000.0);
    quantitySpin_->setDecimals(3);

    barcodeEdit_ = new QLineEdit(this);
    barcodeEdit_->setReadOnly(true);
    barcodeEdit_->setPlaceholderText("Press Generate to allocate");

    generateButton_ = new QPushButton("Generate Barcode", this);
    auto *barcodeRow = new QHBoxLayout();
    barcodeRow->addWidget(barcodeEdit_, 1);
    barcodeRow->addWidget(generateButton_);

    form->addRow("Name", nameEdit_);
    form->addRow("Acronym", acronymEdit_);
    form->addRow("Category", categoryCombo_);
    form->addRow("Sold by", unitCombo_);
    form->addRow("Unit price", priceSpin_);
    form->addRow("Tax (PKR)", taxSpin_);
    form->addRow("Discount/unit", discountSpin_);
    form->addRow("In stock", quantitySpin_);
    form->addRow("Barcode", barcodeRow);
    layout->addLayout(form);

    auto *buttons = new QHBoxLayout();
    auto *save = new QPushButton("Save", this);
    auto *cancel = new QPushButton("Cancel", this);
    buttons->addStretch();
    buttons->addWidget(save);
    buttons->addWidget(cancel);
    layout->addLayout(buttons);

    connect(generateButton_, &QPushButton::clicked, this,
            &ProductEditorDialog::generateBarcode);
    connect(save, &QPushButton::clicked, this, &ProductEditorDialog::onSave);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    if (existingId_ > 0)
    {
        generateButton_->setEnabled(false); // barcode is fixed for existing products
        categoryCombo_->setEnabled(false);  // category is encoded in the barcode
    }
}

void ProductEditorDialog::populateFromExisting()
{
    const Product *p = catalog_->findById(existingId_);
    if (p == nullptr) return;

    nameEdit_->setText(QString::fromStdString(p->name()));
    acronymEdit_->setText(QString::fromStdString(p->acronym()));
    const int idx = categoryCombo_->findData(
        QString::fromStdString(p->categoryId()));
    if (idx >= 0) categoryCombo_->setCurrentIndex(idx);
    unitCombo_->setCurrentIndex(p->unit() == MeasurementUnit::Count ? 0 : 1);
    priceSpin_->setValue(p->unitPrice());
    taxSpin_->setValue(p->taxAmount());
    discountSpin_->setValue(p->discountPerUnit());
    quantitySpin_->setValue(p->availableQuantity());
    barcodeEdit_->setText(QString::fromStdString(p->barcode()));
    generatedBarcode_ = p->barcode();
}

void ProductEditorDialog::generateBarcode()
{
    if (categoryCombo_->currentIndex() < 0)
    {
        QMessageBox::warning(this, "Generate Barcode",
                             "Add a category first.");
        return;
    }
    const std::string categoryId =
        categoryCombo_->currentData().toString().toStdString();

    std::vector<std::string> existing;
    for (const Product &p : catalog_->all())
    {
        existing.push_back(p.barcode());
    }

    try
    {
        const std::string barcode = BarcodeAllocator::next(categoryId, existing);
        generatedBarcode_ = barcode;
        barcodeEdit_->setText(QString::fromStdString(barcode));
    }
    catch (const std::exception &e)
    {
        QMessageBox::warning(this, "Generate Barcode", e.what());
    }
}

void ProductEditorDialog::onSave()
{
    if (nameEdit_->text().trimmed().isEmpty() ||
        acronymEdit_->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Save", "Name and acronym are required.");
        return;
    }
    if (generatedBarcode_.empty())
    {
        QMessageBox::warning(this, "Save", "Generate a barcode first.");
        return;
    }
    if (persist(generatedBarcode_))
    {
        accept();
    }
}

bool ProductEditorDialog::persist(const std::string &barcode)
{
    try
    {
        const int newId = existingId_ > 0
                              ? existingId_
                              : ([this]() {
                                    int maxId = 0;
                                    for (const Product &p : catalog_->all())
                                        maxId = std::max(maxId, p.id());
                                    return maxId + 1;
                                })();

        Product p(newId,
                  barcode,
                  categoryCombo_->currentData().toString().toStdString(),
                  acronymEdit_->text().trimmed().toStdString(),
                  nameEdit_->text().trimmed().toStdString(),
                  unitCombo_->currentIndex() == 0 ? MeasurementUnit::Count
                                                  : MeasurementUnit::Weight,
                  priceSpin_->value(),
                  taxSpin_->value(),
                  discountSpin_->value(),
                  quantitySpin_->value(),
                  "barcodes/" + barcode + ".png");

        if (existingId_ > 0)
        {
            catalog_->updateProduct(p);
        }
        else
        {
            catalog_->addProduct(p);

            // Write the image only after the product is accepted, so a
            // cancelled dialog does not leave an orphan PNG behind.
            BarcodeWidget widget;
            widget.setPattern(BarcodeEncoder().encode(std::stoi(barcode)));
            QDir().mkpath("barcodes");
            const QString path = QString("barcodes/%1.png")
                                     .arg(QString::fromStdString(barcode));
            if (!widget.renderImage().save(path))
            {
                QMessageBox::warning(this, "Barcode Image",
                    "The product was saved, but its barcode image could not be written.");
            }
        }
        return true;
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "Save", e.what());
        return false;
    }
}