#ifndef ORDERRECORDINGSCREEN_H
#define ORDERRECORDINGSCREEN_H

#include <QWidget>
#include <vector>
#include "../models/Order.h"
#include "../models/Product.h"

class QTableWidget;
class QLineEdit;
class QComboBox;
class QLabel;
class QPushButton;
class QVideoWidget;
class CameraHandler;
class ProductManager;
class BarcodeImageScanner;
class BarcodeDecoder;

class OrderRecordingScreen : public QWidget {
    Q_OBJECT

public:
    explicit OrderRecordingScreen(QWidget *parent = nullptr);
    ~OrderRecordingScreen();

signals:
    void orderSaved(); 

private slots:
    void startCamera();
    void showDescription();
    void generateBill();
    void manualRefresh();
    void clearScreen();
    void showBillDialog(const Order& o, const std::vector<OrderDetail>& details, const std::vector<Product>& products); // <--- ADDED

private:
    void scanFrame();
    void handleScannedBarcode(const QString& barcode);
    void updateCartUI();
    void refreshRecentOrders();

    // UI Elements
    QVideoWidget *cameraPreview;
    QLabel *statusLabel;
    QTableWidget *cartTable;
    QLineEdit *custNameInput;
    QLineEdit *contactInput;
    QComboBox *paymentModeInput;
    QLineEdit *merchantInput;
    
    // Recent Orders Table
    QTableWidget *recentOrdersTable;

    // Backend
    CameraHandler *cameraHandler;
    ProductManager *productMgr;
    BarcodeImageScanner *scanner;
    BarcodeDecoder *decoder;

    // State
    std::vector<OrderDetail> cartDetails;
    std::vector<Product> cartProducts;
};

#endif