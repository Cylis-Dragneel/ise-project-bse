#ifndef REPORTS_DIALOG_H
#define REPORTS_DIALOG_H

#include <QDialog>
#include <string>

class ProductCatalog;
class SeasonService;
class QComboBox;
class QDateEdit;
class QLabel;
class QTableWidget;

// Manager reports (requirement 11). One tab per report; data comes from the
// Qt-free ReportsService, so no report logic lives here.
class ReportsDialog : public QDialog
{
    Q_OBJECT
public:
    ReportsDialog(const ProductCatalog &catalog,
                  const std::string &orderPath,
                  const std::string &detailPath,
                  const std::string &billPath,
                  const std::string &billDetailPath,
                  const SeasonService *seasons,
                  QWidget *parent = nullptr);

private:
    const ProductCatalog &catalog_;
    std::string orderPath_, detailPath_, billPath_, billDetailPath_;
    const SeasonService *seasons_;

    QTableWidget *incompleteTable_ = nullptr;
    QLabel *incompleteTotal_ = nullptr;
    QTableWidget *completedTable_ = nullptr;
    QLabel *completedTotal_ = nullptr;
    QComboBox *cashPeriod_ = nullptr;
    QDateEdit *cashFrom_ = nullptr;
    QDateEdit *cashTo_ = nullptr;
    QTableWidget *cashTable_ = nullptr;
    QLabel *cashTotal_ = nullptr;
    QTableWidget *allSalesTable_ = nullptr;
    QLabel *allSalesTotals_ = nullptr;
    QTableWidget *topSoldTable_ = nullptr;
    QTableWidget *topStockTable_ = nullptr;

    QWidget *buildIncompleteTab();
    QWidget *buildCompletedTab();
    QWidget *buildCashTab();
    QWidget *buildAllSalesTab();
    QWidget *buildTopSoldTab();
    QWidget *buildTopStockTab();

    void refreshIncomplete();
    void refreshCompleted();
    void refreshCash();
    void refreshAllSales();
    void refreshTopSold();
    void refreshTopStock();
};

#endif // REPORTS_DIALOG_H