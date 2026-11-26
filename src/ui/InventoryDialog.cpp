#include "InventoryDialog.h"

#include "catalog/ProductCatalog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <exception>

InventoryDialog::InventoryDialog(ProductCatalog *catalog, QWidget *parent)
    : QDialog(parent), catalog_(catalog)
{
    setWindowTitle("Inventory");
    resize(720, 480);

    table_ = new QTableWidget(0, 5, this);
    table_->setHorizontalHeaderLabels(
        {"ID", "Name", "Barcode", "Unit", "In Stock"});
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);

    auto *editBtn = new QPushButton("Set Qty...", this);
    auto *closeBtn = new QPushButton("Close", this);

    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(editBtn, QDialogButtonBox::ActionRole);
    buttons->addButton(closeBtn, QDialogButtonBox::RejectRole);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(table_, 1);
    layout->addWidget(buttons);

    connect(editBtn, &QPushButton::clicked, this, [this]() {
        applyEdit(table_->currentRow());
    });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(table_, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int) { applyEdit(row); });

    refresh();
}

void InventoryDialog::refresh()
{
    table_->setRowCount(0);
    int row = 0;
    for (const Product &p : catalog_->all())
    {
        table_->insertRow(row);
        table_->setItem(row, 0, new QTableWidgetItem(QString::number(p.id())));
        table_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p.name())));
        table_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(p.barcode())));
        table_->setItem(row, 3, new QTableWidgetItem(
            p.unit() == MeasurementUnit::Count ? "count" : "weight"));
        table_->setItem(row, 4, new QTableWidgetItem(
            QString::number(p.availableQuantity())));
        ++row;
    }
}

void InventoryDialog::applyEdit(int row)
{
    if (row < 0 || row >= static_cast<int>(catalog_->all().size())) return;

    const Product &p = catalog_->all()[row];

    QDialog dlg(this);
    dlg.setWindowTitle("Set Quantity");
    auto *lay = new QVBoxLayout(&dlg);
    auto *spin = new QDoubleSpinBox(&dlg);
    spin->setRange(0.0, 1000000000.0);
    spin->setDecimals(p.unit() == MeasurementUnit::Count ? 0 : 3);
    spin->setValue(p.availableQuantity());
    lay->addWidget(spin);
    auto *ok = new QPushButton("Save", &dlg);
    lay->addWidget(ok);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    if (dlg.exec() != QDialog::Accepted) return;

    try
    {
        catalog_->setStock(p.barcode(), spin->value());
        refresh();
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "Set Quantity", e.what());
    }
}