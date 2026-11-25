#ifndef POSWINDOW_H
#define POSWINDOW_H

#include <QWidget>
#include <QString>

#include <optional>
#include <string>
#include <vector>

#include "billing/BillingService.h"
#include "catalog/FileProductRepository.h"
#include "catalog/ProductCatalog.h"
#include "catalog/ProductResolver.h"
#include "seasons/FileSeasonRepository.h"
#include "seasons/SeasonService.h"
#include "Order.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTextEdit;
class QVideoWidget;
class CameraHandler;
class BarcodeScannerService;

// The Point of Sale screen: role selection, order entry, product description
// and bill display. All business logic comes from the Qt-free core:
//   ProductResolver  -> barcode -> product + description + message
//   BillingService   -> cart -> totals -> Bill.txt
//   OrderManager     -> Order.txt / OrderDetail.txt
//   BarcodeScannerService -> live camera frames
class PosWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PosWindow(QWidget *parent = nullptr);
    ~PosWindow() override;

    // Called by MainWindow when another tab takes over the camera.
    void stopCamera();
    void addScannedBarcode(const QString &barcode);

signals:
    void orderSaved();

private:
    enum class Role
    {
        Cashier,
        Manager
    };

    struct CartLine
    {
        Product product;
        double quantity = 1;
    };

    // --- setup ---
    QWidget *createRolePage();
    QWidget *createPosPage();
    QWidget *createManagerPage();
    void selectRole(Role role);

    // --- cart ---
    bool addBarcode(const std::string &barcode, double quantity);
    void addFromInput();
    void setSelectedQuantity();
    void removeSelected();
    void updateCart();
    void updateTotals();
    void showDescription();
    int selectedRow() const;

    // --- order / bill ---
    void generateBill();
    void showBill(const Order &order, const Bill &bill,
                  const std::vector<CartLine> &lines);
    void refreshRecentOrders();

    // --- camera ---
    void startOrArmCamera();
    void stopCameraInternal();

    // --- helpers ---
    void setStatus(const QString &text);
    void configureQuantitySpin(QDoubleSpinBox *spin, const Product &product);

    std::string productPath_;
    std::string orderPath_;
    std::string detailPath_;
    std::string billPath_;
    std::string billDetailPath_;
    std::string seasonPath_;
    std::string catalogError_;

    ProductCatalog catalog_;
    ProductResolver resolver_;
    SeasonService seasons_;
    BillingService billing_;

    std::vector<CartLine> cart_;
    std::optional<Product> lastProduct_;
    Role role_ = Role::Cashier;

    QStackedWidget *stack_ = nullptr;
    QLabel *roleLabel_ = nullptr;
    QWidget *managerTools_ = nullptr;

    QLineEdit *barcodeInput_ = nullptr;
    QDoubleSpinBox *quantityInput_ = nullptr;
    QDoubleSpinBox *selectedQuantityInput_ = nullptr;
    QTableWidget *cartTable_ = nullptr;
    QTextEdit *specsView_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QLabel *totalsLabel_ = nullptr;
    QPushButton *billButton_ = nullptr;

    QLineEdit *customerInput_ = nullptr;
    QLineEdit *contactInput_ = nullptr;
    QComboBox *paymentModeInput_ = nullptr;
    QLineEdit *merchantInput_ = nullptr;

    QTableWidget *recentTable_ = nullptr;

    QVideoWidget *cameraPreview_ = nullptr;
    QLabel *cameraPlaceholder_ = nullptr;
    CameraHandler *cameraHandler_ = nullptr;
    BarcodeScannerService *scanner_ = nullptr;
    QPushButton *cameraButton_ = nullptr;
    QPushButton *stopCameraButton_ = nullptr;
};

#endif // POSWINDOW_H
