#include "ReportsDialog.h"

#include "reports/ReportsService.h"
#include "catalog/ProductCatalog.h"
#include "catalog/NumberFormat.h"
#include "seasons/SeasonService.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include <exception>

namespace {

QTableWidget *makeTable(QWidget *parent, const QStringList &headers)
{
    auto *t = new QTableWidget(0, headers.size(), parent);
    t->setHorizontalHeaderLabels(headers);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setAlternatingRowColors(true);
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setStretchLastSection(true);
    return t;
}

void addRow(QTableWidget *t, int row,
            std::initializer_list<QString> cells)
{
    int col = 0;
    for (const QString &text : cells) {
        t->setItem(row, col++, new QTableWidgetItem(text));
    }
}

QString money(double v)
{
    return QString::fromStdString(NumberFormat::money(v));
}

} // namespace

ReportsDialog::ReportsDialog(const ProductCatalog &catalog,
                             const std::string &orderPath,
                             const std::string &detailPath,
                             const std::string &billPath,
                             const std::string &billDetailPath,
                             const SeasonService *seasons,
                             QWidget *parent)
    : QDialog(parent),
      catalog_(catalog),
      orderPath_(orderPath), detailPath_(detailPath),
      billPath_(billPath), billDetailPath_(billDetailPath),
      seasons_(seasons)
{
    setWindowTitle("Manager Reports");
    resize(900, 620);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(buildIncompleteTab(), "Incomplete Orders");
    tabs->addTab(buildCompletedTab(), "Completed Orders");
    tabs->addTab(buildCashTab(), "Cash Sales");
    tabs->addTab(buildAllSalesTab(), "All Sales");
    tabs->addTab(buildTopSoldTab(), "Top Sold");
    tabs->addTab(buildTopStockTab(), "Top Stock");

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs, 1);

    auto *close = new QPushButton("Close", this);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(close);

    refreshIncomplete();
    refreshCompleted();
    refreshCash();
    refreshAllSales();
    refreshTopSold();
    refreshTopStock();
}

QWidget *ReportsDialog::buildIncompleteTab()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    incompleteTable_ = makeTable(page, {"Order ID", "Date", "Customer", "Status", "Total"});
    incompleteTotal_ = new QLabel(page);
    auto *refresh = new QPushButton("Refresh", page);
    connect(refresh, &QPushButton::clicked, this, &ReportsDialog::refreshIncomplete);
    lay->addWidget(incompleteTable_, 1);
    auto *row = new QHBoxLayout();
    row->addWidget(incompleteTotal_, 1);
    row->addWidget(refresh);
    lay->addLayout(row);
    return page;
}

QWidget *ReportsDialog::buildCompletedTab()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    completedTable_ = makeTable(page, {"Order ID", "Date", "Customer", "Status", "Total"});
    completedTotal_ = new QLabel(page);
    auto *refresh = new QPushButton("Refresh", page);
    connect(refresh, &QPushButton::clicked, this, &ReportsDialog::refreshCompleted);
    lay->addWidget(completedTable_, 1);
    auto *row = new QHBoxLayout();
    row->addWidget(completedTotal_, 1);
    row->addWidget(refresh);
    lay->addLayout(row);
    return page;
}

QWidget *ReportsDialog::buildCashTab()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);

    auto *controls = new QHBoxLayout();
    cashPeriod_ = new QComboBox(page);
    cashPeriod_->addItems({"Daily", "Weekly", "Monthly", "Yearly", "Custom"});
    cashFrom_ = new QDateEdit(QDate::currentDate(), page);
    cashFrom_->setCalendarPopup(true);
    cashFrom_->setDisplayFormat("yyyy-MM-dd");
    cashTo_ = new QDateEdit(QDate::currentDate(), page);
    cashTo_->setCalendarPopup(true);
    cashTo_->setDisplayFormat("yyyy-MM-dd");
    cashFrom_->setEnabled(false);
    cashTo_->setEnabled(false);

    controls->addWidget(new QLabel("Period:", page));
    controls->addWidget(cashPeriod_);
    controls->addWidget(new QLabel("From:", page));
    controls->addWidget(cashFrom_);
    controls->addWidget(new QLabel("To:", page));
    controls->addWidget(cashTo_);
    controls->addStretch();
    auto *refresh = new QPushButton("Refresh", page);
    controls->addWidget(refresh);
    lay->addLayout(controls);

    cashTable_ = makeTable(page, {"Period", "Orders", "Amount"});
    cashTotal_ = new QLabel(page);
    lay->addWidget(cashTable_, 1);
    lay->addWidget(cashTotal_);

    connect(cashPeriod_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                const bool custom = (idx == 4);
                cashFrom_->setEnabled(custom);
                cashTo_->setEnabled(custom);
            });
    connect(refresh, &QPushButton::clicked, this, &ReportsDialog::refreshCash);
    return page;
}

QWidget *ReportsDialog::buildAllSalesTab()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    allSalesTable_ = makeTable(page,
        {"Bill ID", "Date", "Customer", "Status",
         "No Tax", "Tax", "Product Disc", "Seasonal Disc",
         "Special Disc", "Payable"});
    allSalesTotals_ = new QLabel(page);
    auto *refresh = new QPushButton("Refresh", page);
    connect(refresh, &QPushButton::clicked, this, &ReportsDialog::refreshAllSales);
    lay->addWidget(allSalesTable_, 1);
    auto *row = new QHBoxLayout();
    row->addWidget(allSalesTotals_, 1);
    row->addWidget(refresh);
    lay->addLayout(row);
    return page;
}

QWidget *ReportsDialog::buildTopSoldTab()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    topSoldTable_ = makeTable(page, {"Acronym", "Name", "Sold Qty", "Remaining Stock"});
    auto *refresh = new QPushButton("Refresh", page);
    connect(refresh, &QPushButton::clicked, this, &ReportsDialog::refreshTopSold);
    lay->addWidget(topSoldTable_, 1);
    lay->addWidget(refresh);
    return page;
}

QWidget *ReportsDialog::buildTopStockTab()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    topStockTable_ = makeTable(page, {"Acronym", "Name", "Remaining Stock"});
    auto *refresh = new QPushButton("Refresh", page);
    connect(refresh, &QPushButton::clicked, this, &ReportsDialog::refreshTopStock);
    lay->addWidget(topStockTable_, 1);
    lay->addWidget(refresh);
    return page;
}

void ReportsDialog::refreshIncomplete()
{
    try {
        reports::ReportsService svc(catalog_, orderPath_, detailPath_,
                                    billPath_, billDetailPath_, seasons_);
        double total = 0;
        const auto rows = svc.incompleteOrders(&total);
        incompleteTable_->setRowCount(0);
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const auto &r = rows[i];
            incompleteTable_->insertRow(static_cast<int>(i));
            addRow(incompleteTable_, static_cast<int>(i), {
                QString::fromStdString(r.orderId),
                QString::fromStdString(r.date),
                QString::fromStdString(r.customerName),
                QString::fromStdString(r.status),
                money(r.total)
            });
        }
        incompleteTotal_->setText(QString("Total amount: PKR %1").arg(money(total)));
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Incomplete Orders", e.what());
    }
}

void ReportsDialog::refreshCompleted()
{
    try {
        reports::ReportsService svc(catalog_, orderPath_, detailPath_,
                                    billPath_, billDetailPath_, seasons_);
        double total = 0;
        const auto rows = svc.completedOrders(&total);
        completedTable_->setRowCount(0);
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const auto &r = rows[i];
            completedTable_->insertRow(static_cast<int>(i));
            addRow(completedTable_, static_cast<int>(i), {
                QString::fromStdString(r.orderId),
                QString::fromStdString(r.date),
                QString::fromStdString(r.customerName),
                QString::fromStdString(r.status),
                money(r.total)
            });
        }
        completedTotal_->setText(QString("Total amount: PKR %1").arg(money(total)));
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Completed Orders", e.what());
    }
}

void ReportsDialog::refreshCash()
{
    try {
        reports::ReportsService svc(catalog_, orderPath_, detailPath_,
                                    billPath_, billDetailPath_, seasons_);
        reports::CashPeriod period = reports::CashPeriod::Daily;
        switch (cashPeriod_->currentIndex()) {
        case 0: period = reports::CashPeriod::Daily; break;
        case 1: period = reports::CashPeriod::Weekly; break;
        case 2: period = reports::CashPeriod::Monthly; break;
        case 3: period = reports::CashPeriod::Yearly; break;
        case 4: period = reports::CashPeriod::Custom; break;
        }
        const QString from = cashFrom_->date().toString("yyyy-MM-dd");
        const QString to = cashTo_->date().toString("yyyy-MM-dd");
        double total = 0;
        const auto rows = svc.cashSales(period, from.toStdString(), to.toStdString(),
                                        std::string(), &total);
        cashTable_->setRowCount(0);
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const auto &r = rows[i];
            cashTable_->insertRow(static_cast<int>(i));
            addRow(cashTable_, static_cast<int>(i), {
                QString::fromStdString(r.period),
                QString::number(r.orderCount),
                money(r.amount)
            });
        }
        cashTotal_->setText(QString("Total cash: PKR %1").arg(money(total)));
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Cash Sales", e.what());
    }
}

void ReportsDialog::refreshAllSales()
{
    try {
        reports::ReportsService svc(catalog_, orderPath_, detailPath_,
                                    billPath_, billDetailPath_, seasons_);
        reports::SalesTotals totals;
        const auto rows = svc.allSales(&totals);
        allSalesTable_->setRowCount(0);
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const auto &r = rows[i];
            allSalesTable_->insertRow(static_cast<int>(i));
            addRow(allSalesTable_, static_cast<int>(i), {
                QString::fromStdString(r.billId),
                QString::fromStdString(r.date),
                QString::fromStdString(r.customerName),
                QString::fromStdString(r.paymentStatus),
                money(r.totalNoTax),
                money(r.totalTax),
                money(r.productDiscount),
                money(r.seasonalDiscount),
                money(r.specialDiscount),
                money(r.totalPayable)
            });
        }
        allSalesTotals_->setText(QString(
            "Total payable: PKR %1   |   Product disc: PKR %2   |   "
            "Seasonal: PKR %3   |   Special: PKR %4")
            .arg(money(totals.totalPayable),
                 money(totals.productDiscount),
                 money(totals.seasonalDiscount),
                 money(totals.specialDiscount)));
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "All Sales", e.what());
    }
}

void ReportsDialog::refreshTopSold()
{
    try {
        reports::ReportsService svc(catalog_, orderPath_, detailPath_,
                                    billPath_, billDetailPath_, seasons_);
        const auto rows = svc.topSoldItems(10);
        topSoldTable_->setRowCount(0);
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const auto &r = rows[i];
            topSoldTable_->insertRow(static_cast<int>(i));
            addRow(topSoldTable_, static_cast<int>(i), {
                QString::fromStdString(r.acronym),
                QString::fromStdString(r.name),
                QString::fromStdString(NumberFormat::general(r.quantity)),
                QString::fromStdString(NumberFormat::general(r.remainingStock))
            });
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Top Sold", e.what());
    }
}

void ReportsDialog::refreshTopStock()
{
    try {
        reports::ReportsService svc(catalog_, orderPath_, detailPath_,
                                    billPath_, billDetailPath_, seasons_);
        const auto rows = svc.topStockItems(10);
        topStockTable_->setRowCount(0);
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const auto &r = rows[i];
            topStockTable_->insertRow(static_cast<int>(i));
            addRow(topStockTable_, static_cast<int>(i), {
                QString::fromStdString(r.acronym),
                QString::fromStdString(r.name),
                QString::fromStdString(NumberFormat::general(r.remainingStock))
            });
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Top Stock", e.what());
    }
}