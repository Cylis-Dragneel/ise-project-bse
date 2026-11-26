#include "OrdersDialog.h"

#include "OrderManager.h"
#include "catalog/NumberFormat.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cctype>

namespace {

std::string toLowerText(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

} // namespace

OrdersDialog::OrdersDialog(const std::string &orderPath, QWidget *parent)
    : QDialog(parent), orderPath_(orderPath)
{
    setWindowTitle("Open Order");
    resize(760, 460);

    auto *layout = new QVBoxLayout(this);

    auto *searchRow = new QHBoxLayout();
    searchRow->addWidget(new QLabel("Order ID or customer:", this));
    search_ = new QLineEdit(this);
    search_->setPlaceholderText("Type to filter");
    searchRow->addWidget(search_, 1);
    layout->addLayout(searchRow);

    table_ = new QTableWidget(0, 5, this);
    table_->setHorizontalHeaderLabels(
        {"Order ID", "Date", "Customer", "Status", "Amount"});
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table_, 1);

    auto *buttons = new QHBoxLayout();
    auto *open = new QPushButton("Open", this);
    auto *cancel = new QPushButton("Cancel", this);
    buttons->addStretch();
    buttons->addWidget(open);
    buttons->addWidget(cancel);
    layout->addLayout(buttons);

    connect(search_, &QLineEdit::textChanged, this, [this]() { refresh(); });
    connect(open, &QPushButton::clicked, this, &OrdersDialog::openSelected);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(table_, &QTableWidget::cellDoubleClicked, this,
            [this](int, int) { openSelected(); });

    refresh();
}

void OrdersDialog::refresh()
{
    const std::string filter = toLowerText(search_->text().trimmed().toStdString());
    const std::vector<Order> orders = OrderManager::loadOrders(orderPath_);

    table_->setRowCount(0);
    int row = 0;
    for (const Order &order : orders)
    {
        if (!filter.empty())
        {
            const bool idMatch =
                toLowerText(order.orderId).find(filter) != std::string::npos;
            const bool customerMatch =
                toLowerText(order.customerName).find(filter) != std::string::npos;
            if (!idMatch && !customerMatch)
            {
                continue;
            }
        }

        table_->insertRow(row);
        auto *idItem = new QTableWidgetItem(QString::fromStdString(order.orderId));
        idItem->setData(Qt::UserRole, QString::fromStdString(order.orderId));
        table_->setItem(row, 0, idItem);
        table_->setItem(row, 1, new QTableWidgetItem(
            QString::fromStdString(order.date)));
        table_->setItem(row, 2, new QTableWidgetItem(
            QString::fromStdString(order.customerName)));
        table_->setItem(row, 3, new QTableWidgetItem(
            QString::fromStdString(order.status)));
        table_->setItem(row, 4, new QTableWidgetItem(
            QString::fromStdString(NumberFormat::money(order.paymentAmount))));
        ++row;
    }

    if (table_->rowCount() > 0)
    {
        table_->selectRow(0);
    }
}

void OrdersDialog::openSelected()
{
    const int row = table_->currentRow();
    if (row < 0 || row >= table_->rowCount())
    {
        return;
    }

    const QTableWidgetItem *item = table_->item(row, 0);
    if (item == nullptr)
    {
        return;
    }

    selectedOrderId_ = item->data(Qt::UserRole).toString().toStdString();
    accept();
}
