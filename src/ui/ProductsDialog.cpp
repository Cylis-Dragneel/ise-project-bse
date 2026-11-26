#include "ProductsDialog.h"

#include "ProductEditorDialog.h"
#include "catalog/CategoryCatalog.h"
#include "catalog/ProductCatalog.h"
#include "catalog/ProductCategory.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include <exception>

ProductsDialog::ProductsDialog(ProductCatalog *catalog,
                               CategoryCatalog *categories,
                               QWidget *parent)
    : QDialog(parent), catalog_(catalog), categories_(categories)
{
    setWindowTitle("Products & Categories");
    resize(820, 560);

    auto *tabs = new QTabWidget(this);

    // Products tab
    auto *productsPage = new QWidget(this);
    auto *pLay = new QVBoxLayout(productsPage);
    productTable_ = new QTableWidget(0, 5, productsPage);
    productTable_->setHorizontalHeaderLabels(
        {"ID", "Name", "Barcode", "Category", "Price"});
    productTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    productTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    productTable_->verticalHeader()->setVisible(false);
    productTable_->horizontalHeader()->setStretchLastSection(true);
    auto *pButtons = new QDialogButtonBox(productsPage);
    auto *addP = new QPushButton("Add Product", productsPage);
    auto *editP = new QPushButton("Edit Product", productsPage);
    pButtons->addButton(addP, QDialogButtonBox::ActionRole);
    pButtons->addButton(editP, QDialogButtonBox::ActionRole);
    pLay->addWidget(productTable_, 1);
    pLay->addWidget(pButtons);
    tabs->addTab(productsPage, "Products");

    // Categories tab
    auto *catPage = new QWidget(this);
    auto *cLay = new QVBoxLayout(catPage);
    categoryTable_ = new QTableWidget(0, 3, catPage);
    categoryTable_->setHorizontalHeaderLabels({"ID", "Name", "Acronym"});
    categoryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    categoryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    categoryTable_->verticalHeader()->setVisible(false);
    categoryTable_->horizontalHeader()->setStretchLastSection(true);
    auto *cButtons = new QDialogButtonBox(catPage);
    auto *addC = new QPushButton("Add Category", catPage);
    auto *editC = new QPushButton("Edit Category", catPage);
    cButtons->addButton(addC, QDialogButtonBox::ActionRole);
    cButtons->addButton(editC, QDialogButtonBox::ActionRole);
    cLay->addWidget(categoryTable_, 1);
    cLay->addWidget(cButtons);
    tabs->addTab(catPage, "Categories");

    auto *closeBtn = new QPushButton("Close", this);
    auto *outer = new QVBoxLayout(this);
    outer->addWidget(tabs, 1);
    outer->addWidget(closeBtn);

    connect(addP, &QPushButton::clicked, this, &ProductsDialog::addProduct);
    connect(editP, &QPushButton::clicked, this, [this]() {
        editProduct(productTable_->currentRow());
    });
    connect(productTable_, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int) { editProduct(row); });
    connect(addC, &QPushButton::clicked, this, &ProductsDialog::addCategory);
    connect(editC, &QPushButton::clicked, this, [this]() {
        editCategory(categoryTable_->currentRow());
    });
    connect(categoryTable_, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int) { editCategory(row); });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    refreshProducts();
    refreshCategories();
}

void ProductsDialog::refreshProducts()
{
    productTable_->setRowCount(0);
    int row = 0;
    for (const Product &p : catalog_->all())
    {
        productTable_->insertRow(row);
        productTable_->setItem(row, 0, new QTableWidgetItem(QString::number(p.id())));
        productTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p.name())));
        productTable_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(p.barcode())));
        productTable_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(p.categoryId())));
        productTable_->setItem(row, 4, new QTableWidgetItem(
            QString::number(p.unitPrice(), 'f', 2)));
        ++row;
    }
}

void ProductsDialog::refreshCategories()
{
    categoryTable_->setRowCount(0);
    int row = 0;
    for (const ProductCategory &c : categories_->all())
    {
        categoryTable_->insertRow(row);
        categoryTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(c.id())));
        categoryTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(c.name())));
        categoryTable_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(c.acronym())));
        ++row;
    }
}

void ProductsDialog::addProduct()
{
    if (categories_->all().empty())
    {
        QMessageBox::information(this, "Add Product",
                                 "Add a category first.");
        return;
    }
    ProductEditorDialog dlg(catalog_, categories_, 0, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        refreshProducts();
    }
}

void ProductsDialog::editProduct(int row)
{
    if (row < 0 || row >= static_cast<int>(catalog_->all().size())) return;
    const int id = catalog_->all()[row].id();
    ProductEditorDialog dlg(catalog_, categories_, id, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        refreshProducts();
    }
}

void ProductsDialog::addCategory()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Add Category");
    auto *form = new QFormLayout(&dlg);
    auto *idEdit = new QLineEdit(&dlg);
    idEdit->setMaxLength(2);
    auto *nameEdit = new QLineEdit(&dlg);
    auto *acrEdit = new QLineEdit(&dlg);
    acrEdit->setMaxLength(8);
    form->addRow("ID (10-99)", idEdit);
    form->addRow("Name", nameEdit);
    form->addRow("Acronym", acrEdit);
    auto *ok = new QPushButton("Save", &dlg);
    form->addRow(ok);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    if (dlg.exec() != QDialog::Accepted) return;

    try
    {
        categories_->add(ProductCategory(
            idEdit->text().toStdString(),
            nameEdit->text().trimmed().toStdString(),
            acrEdit->text().trimmed().toStdString()));
        refreshCategories();
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "Add Category", e.what());
    }
}

void ProductsDialog::editCategory(int row)
{
    if (row < 0 || row >= static_cast<int>(categories_->all().size())) return;
    const ProductCategory &c = categories_->all()[row];

    QDialog dlg(this);
    dlg.setWindowTitle("Edit Category");
    auto *form = new QFormLayout(&dlg);
    auto *idEdit = new QLineEdit(QString::fromStdString(c.id()), &dlg);
    idEdit->setReadOnly(true);
    auto *nameEdit = new QLineEdit(QString::fromStdString(c.name()), &dlg);
    auto *acrEdit = new QLineEdit(QString::fromStdString(c.acronym()), &dlg);
    acrEdit->setMaxLength(8);
    form->addRow("ID", idEdit);
    form->addRow("Name", nameEdit);
    form->addRow("Acronym", acrEdit);
    auto *ok = new QPushButton("Save", &dlg);
    form->addRow(ok);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    if (dlg.exec() != QDialog::Accepted) return;

    try
    {
        categories_->update(ProductCategory(
            c.id(),
            nameEdit->text().trimmed().toStdString(),
            acrEdit->text().trimmed().toStdString()));
        refreshCategories();
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "Edit Category", e.what());
    }
}