#include "SeasonsDialog.h"

#include "seasons/SeasonService.h"

#include <QCheckBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <exception>

namespace {

bool editSeasonDialog(QWidget *parent,
                      QString &start, QString &end,
                      double &percent, bool &active,
                      const QString &title)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    auto *form = new QFormLayout(&dlg);
    auto *startEdit = new QDateEdit(QDate::fromString(start, "yyyy-MM-dd"), &dlg);
    startEdit->setDisplayFormat("yyyy-MM-dd");
    startEdit->setCalendarPopup(true);
    auto *endEdit = new QDateEdit(QDate::fromString(end, "yyyy-MM-dd"), &dlg);
    endEdit->setDisplayFormat("yyyy-MM-dd");
    endEdit->setCalendarPopup(true);
    auto *percentSpin = new QDoubleSpinBox(&dlg);
    percentSpin->setRange(0.0, 100.0);
    percentSpin->setDecimals(2);
    percentSpin->setValue(percent);
    auto *activeCheck = new QCheckBox("Active", &dlg);
    activeCheck->setChecked(active);

    form->addRow("Start date", startEdit);
    form->addRow("End date", endEdit);
    form->addRow("Discount %", percentSpin);
    form->addRow(QString(), activeCheck);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return false;
    start = startEdit->date().toString("yyyy-MM-dd");
    end = endEdit->date().toString("yyyy-MM-dd");
    percent = percentSpin->value();
    active = activeCheck->isChecked();
    return true;
}

} // namespace

SeasonsDialog::SeasonsDialog(SeasonService *seasons, QWidget *parent)
    : QDialog(parent), seasons_(seasons)
{
    setWindowTitle("Seasons & Discounts");
    resize(640, 420);

    table_ = new QTableWidget(0, 4, this);
    table_->setHorizontalHeaderLabels({"Start", "End", "Discount %", "Active"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->verticalHeader()->setVisible(false);

    auto *addBtn = new QPushButton("Add", this);
    auto *editBtn = new QPushButton("Edit", this);
    auto *toggleBtn = new QPushButton("Toggle Active", this);
    auto *deleteBtn = new QPushButton("Delete", this);
    auto *closeBtn = new QPushButton("Close", this);

    auto *buttons = new QHBoxLayout();
    buttons->addWidget(addBtn);
    buttons->addWidget(editBtn);
    buttons->addWidget(toggleBtn);
    buttons->addWidget(deleteBtn);
    buttons->addStretch();
    buttons->addWidget(closeBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(table_, 1);
    layout->addLayout(buttons);

    connect(addBtn, &QPushButton::clicked, this, &SeasonsDialog::addSeason);
    connect(editBtn, &QPushButton::clicked, this, [this]() {
        editSeason(table_->currentRow());
    });
    connect(toggleBtn, &QPushButton::clicked, this, [this]() {
        toggleActive(table_->currentRow());
    });
    connect(deleteBtn, &QPushButton::clicked, this, [this]() {
        removeSeason(table_->currentRow());
    });
    connect(table_, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int) { editSeason(row); });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    refresh();
}

void SeasonsDialog::refresh()
{
    const auto &all = seasons_->all();
    table_->setRowCount(0);
    for (std::size_t i = 0; i < all.size(); ++i) {
        const auto &s = all[i];
        const int row = static_cast<int>(i);
        table_->insertRow(row);
        table_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(s.startDate())));
        table_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(s.endDate())));
        table_->setItem(row, 2, new QTableWidgetItem(QString::number(s.discountPercent())));
        table_->setItem(row, 3, new QTableWidgetItem(s.isActive() ? "Yes" : "No"));
    }
}

void SeasonsDialog::addSeason()
{
    QString start = QDate::currentDate().toString("yyyy-MM-dd");
    QString end = start;
    double pct = 0.0;
    bool active = true;
    if (!editSeasonDialog(this, start, end, pct, active, "Add Season")) return;
    try {
        seasons_->add(Season(start.toStdString(), end.toStdString(), pct, active));
        refresh();
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Add Season", e.what());
    }
}

void SeasonsDialog::editSeason(int row)
{
    if (row < 0 || row >= static_cast<int>(seasons_->all().size())) return;
    const Season &s = seasons_->all()[row];
    QString start = QString::fromStdString(s.startDate());
    QString end = QString::fromStdString(s.endDate());
    double pct = s.discountPercent();
    bool active = s.isActive();
    if (!editSeasonDialog(this, start, end, pct, active, "Edit Season")) return;
    try {
        seasons_->update(static_cast<std::size_t>(row),
                         Season(start.toStdString(), end.toStdString(), pct, active));
        refresh();
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Edit Season", e.what());
    }
}

void SeasonsDialog::toggleActive(int row)
{
    if (row < 0 || row >= static_cast<int>(seasons_->all().size())) return;
    try {
        seasons_->setActive(static_cast<std::size_t>(row),
                            !seasons_->all()[row].isActive());
        refresh();
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Toggle Season", e.what());
    }
}

void SeasonsDialog::removeSeason(int row)
{
    if (row < 0 || row >= static_cast<int>(seasons_->all().size())) return;
    if (QMessageBox::question(this, "Delete Season",
                              "Delete this season?")
        != QMessageBox::Yes) return;
    try {
        seasons_->remove(static_cast<std::size_t>(row));
        refresh();
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Delete Season", e.what());
    }
}