#include "OrderRecordingScreen.h"
#include "ProductDescriptionDialog.h"
#include "../camera/CameraHandler.h"
#include "../decoder/barcodeImageScanner.h"
#include "../decoder/barcodeDecoder.h"
#include "../managers/ProductManager.h"
#include "../managers/OrderManager.h"
#include "../managers/BillManager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QVideoWidget>
#include <QDebug>
#include <QDateTime>
#include <QPainter>
#include <QTimer>
#include <algorithm>
#include <fstream>
#include <QDialog>
#include <QTextEdit>

OrderRecordingScreen::OrderRecordingScreen(QWidget *parent) : QWidget(parent) {
    productMgr = new ProductManager(); 
    scanner = new BarcodeImageScanner();
    decoder = new BarcodeDecoder();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20); 

    // --- Top: Camera & Controls ---
    QHBoxLayout *topLayout = new QHBoxLayout();
    
    cameraPreview = new QVideoWidget();
    cameraPreview->setMinimumSize(400, 300);
    cameraHandler = new CameraHandler(cameraPreview, this);
    
    QVBoxLayout *cameraControls = new QVBoxLayout();
    QPushButton *startCamBtn = new QPushButton("Start Auto-Scan Camera");
    QPushButton *descBtn = new QPushButton("View Description (Last Scanned)");
    statusLabel = new QLabel("Ready. Click Start Camera.");
    
    cameraControls->addWidget(startCamBtn);
    cameraControls->addWidget(descBtn);
    cameraControls->addWidget(statusLabel);
    cameraControls->addStretch();

    topLayout->addWidget(cameraPreview);
    topLayout->addLayout(cameraControls);
    mainLayout->addLayout(topLayout);

    // --- Middle: Cart Table ---
    cartTable = new QTableWidget(0, 4);
    cartTable->setHorizontalHeaderLabels({"Product", "Barcode", "Qty", "Price"});
    cartTable->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(cartTable);

    // --- Recent Orders Table Header with Refresh/Clear Buttons ---
    QHBoxLayout *recentHeaderLayout = new QHBoxLayout();
    
    QLabel *recentLabel = new QLabel("Recent Orders");
    recentLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333; margin-top: 10px;");
    
    QPushButton *clearBtn = new QPushButton("🗑️ Clear Screen");
    clearBtn->setFixedSize(110, 30);
    clearBtn->setStyleSheet("background-color: #fee2e2; color: #991b1b; padding: 5px; font-weight: bold;"); 

    QPushButton *refreshBtn = new QPushButton("🔄 Refresh");
    refreshBtn->setFixedSize(100, 30);
    refreshBtn->setStyleSheet("background-color: #e5e7eb; color: #333; padding: 5px;"); 
    
    recentHeaderLayout->addWidget(recentLabel);
    recentHeaderLayout->addStretch();
    recentHeaderLayout->addWidget(clearBtn);
    recentHeaderLayout->addWidget(refreshBtn);
    mainLayout->addLayout(recentHeaderLayout);

    recentOrdersTable = new QTableWidget(0, 4);
    recentOrdersTable->setHorizontalHeaderLabels({"Order ID", "Customer", "Status", "Amount"});
    recentOrdersTable->horizontalHeader()->setStretchLastSection(true);
    recentOrdersTable->setMaximumHeight(150); 
    mainLayout->addWidget(recentOrdersTable);

    refreshRecentOrders(); 

    // --- Bottom: Customer Info & Generate ---
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    custNameInput = new QLineEdit(); custNameInput->setPlaceholderText("Customer Name");
    contactInput = new QLineEdit(); contactInput->setPlaceholderText("Contact (Optional)");
    paymentModeInput = new QComboBox();
    paymentModeInput->addItems({"cash", "card", "wallet"});
    merchantInput = new QLineEdit(); merchantInput->setPlaceholderText("Merchant (e.g. HBL)");
    QPushButton *billBtn = new QPushButton("Generate Bill");

    bottomLayout->addWidget(custNameInput);
    bottomLayout->addWidget(contactInput);
    bottomLayout->addWidget(paymentModeInput);
    bottomLayout->addWidget(merchantInput);
    bottomLayout->addWidget(billBtn);
    mainLayout->addLayout(bottomLayout);

    // --- Connections ---
    connect(startCamBtn, &QPushButton::clicked, this, &OrderRecordingScreen::startCamera);
    connect(descBtn, &QPushButton::clicked, this, &OrderRecordingScreen::showDescription);
    connect(billBtn, &QPushButton::clicked, this, &OrderRecordingScreen::generateBill);
    connect(refreshBtn, &QPushButton::clicked, this, &OrderRecordingScreen::manualRefresh);
    connect(clearBtn, &QPushButton::clicked, this, &OrderRecordingScreen::clearScreen); 

    // Auto Scan Timer
    QTimer *scanTimer = new QTimer(this);
    connect(scanTimer, &QTimer::timeout, this, &OrderRecordingScreen::scanFrame);
    scanTimer->start(500); 
}

OrderRecordingScreen::~OrderRecordingScreen() {
    if (cameraHandler) cameraHandler->stopCamera();
    delete productMgr;
    delete scanner;
    delete decoder;
}

void OrderRecordingScreen::startCamera() {
    if (!cameraHandler->startCamera()) {
        statusLabel->setText("Failed to start camera.");
    } else {
        statusLabel->setText("Camera running. Point at a barcode...");
    }
}

void OrderRecordingScreen::scanFrame() {
    if (!cameraHandler->isRunning()) return;

    QImage frame = cameraHandler->lastFrameImage();
    if (frame.isNull()) return;

    QImage grayscale = frame.convertToFormat(QImage::Format_Grayscale8);
    auto candidates = scanner->scan(grayscale.constBits(), grayscale.width(), grayscale.height(), grayscale.bytesPerLine(), 128);

    for (const auto& pat : candidates) {
        try {
            int num = decoder->decode(pat);
            QString barcodeStr = QString::number(num);
            handleScannedBarcode(barcodeStr);
            return; 
        } catch (...) {}
    }
}

void OrderRecordingScreen::handleScannedBarcode(const QString& barcode) {
    Product p;
    if (productMgr->findByBarcode(barcode.toStdString(), p)) {
        
        // --- REQUIREMENT 5: CHECK AVAILABLE QUANTITY ---
        int currentInCart = 0;
        for (const auto& d : cartDetails) {
            if (d.productId == p.productId) {
                currentInCart += d.quantity;
            }
        }

        if (currentInCart + 1 > p.availableQty) {
            statusLabel->setText("Error: Not enough stock for " + QString::fromStdString(p.name));
            return;
        }

        // Add to cart
        OrderDetail d;
        d.orderId = ""; 
        d.productId = p.productId;
        d.barcode = p.barcode;
        d.quantity = 1;

        bool found = false;
        for (auto& existing : cartDetails) {
            if (existing.productId == p.productId) {
                existing.quantity++;
                found = true;
                break;
            }
        }
        if (!found) {
            cartDetails.push_back(d);
            cartProducts.push_back(p);
        }
        
        statusLabel->setText("Scanned: " + QString::fromStdString(p.name).replace("_", " "));
        updateCartUI();
    } else {
        statusLabel->setText("Scanned barcode " + barcode + " not found in Product.txt.");
    }
}

void OrderRecordingScreen::updateCartUI() {
    cartTable->setRowCount(0);
    for (size_t i = 0; i < cartDetails.size(); ++i) {
        cartTable->insertRow(i);
        cartTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(cartProducts[i].name).replace("_", " ")));
        cartTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(cartDetails[i].barcode)));
        cartTable->setItem(i, 2, new QTableWidgetItem(QString::number(cartDetails[i].quantity)));
        cartTable->setItem(i, 3, new QTableWidgetItem(QString::number(cartProducts[i].unitPrice * cartDetails[i].quantity, 'f', 2)));
    }
}

void OrderRecordingScreen::showDescription() {
    if (cartProducts.empty()) {
        statusLabel->setText("Cart empty. Scan a product first.");
        return;
    }
    ProductDescriptionDialog dlg(cartProducts.back(), this);
    dlg.exec();
}

void OrderRecordingScreen::generateBill() {
    if (cartDetails.empty()) {
        QMessageBox::warning(this, "Error", "Cart is empty.");
        return;
    }
    if (custNameInput->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Enter customer name.");
        return;
    }

    Order o;
    o.date = OrderManager::getCurrentDate();
    o.time = OrderManager::getCurrentTime();
    o.orderId = OrderManager::generateOrderId(o.date);
    o.customerName = custNameInput->text().toStdString();
    o.contact = contactInput->text().toStdString();
    o.paymentMode = paymentModeInput->currentText().toStdString();
    o.merchant = merchantInput->text().toStdString();
    o.status = "paid"; 

    double total = 0;
    for (size_t i = 0; i < cartDetails.size(); ++i) {
        cartDetails[i].orderId = o.orderId; 
        total += (cartProducts[i].unitPrice + cartProducts[i].tax - cartProducts[i].discount) * cartDetails[i].quantity;
    }
    o.paymentAmount = total;

    if (!OrderManager::saveOrderTransaction(o, cartDetails)) {
        QMessageBox::critical(this, "Error", "Failed to save Order. Check console.");
        return;
    }

    BillManager billMgr;
    std::vector<Bill> bills;
    std::string billId = billMgr.generateBillId();
    
    for (size_t i = 0; i < cartDetails.size(); ++i) {
        Bill b;
        b.billId = billId;
        b.customerName = o.customerName;
        b.orderId = o.orderId;
        b.productAcronym = cartProducts[i].acronym;
        b.unitPriceNoTax = cartProducts[i].unitPrice;
        b.discountPerUnit = cartProducts[i].discount;
        b.quantity = cartDetails[i].quantity;
        b.totalNoTax = b.unitPriceNoTax * b.quantity;
        b.totalTax = cartProducts[i].tax * b.quantity;
        b.totalDiscount = b.discountPerUnit * b.quantity;
        b.totalPayable = b.totalNoTax + b.totalTax - b.totalDiscount;
        b.paymentStatus = o.status;
        bills.push_back(b);
    }

    if (!billMgr.appendBills(bills)) {
        QMessageBox::critical(this, "Error", "Failed to save Bill.txt");
        return;
    }

    // --- REQUIREMENT 10: SHOW BILL ON SCREEN ---
    showBillDialog(o, cartDetails, cartProducts);
    
    cartDetails.clear();
    cartProducts.clear();
    updateCartUI();
    statusLabel->setText("Bill generated. Cart reset.");
    
    refreshRecentOrders(); 
    emit orderSaved(); 
}

void OrderRecordingScreen::showBillDialog(const Order& o, const std::vector<OrderDetail>& details, const std::vector<Product>& products) {
    QDialog *billDialog = new QDialog(this);
    billDialog->setWindowTitle("Bill Receipt");
    billDialog->resize(450, 550);
    QVBoxLayout *layout = new QVBoxLayout(billDialog);

    QTextEdit *textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    
    QString billText;
    billText += "========================================\n";
    billText += "              PAZARLA POS               \n";
    billText += "========================================\n";
    billText += "Order ID: " + QString::fromStdString(o.orderId) + "\n";
    billText += "Customer: " + QString::fromStdString(o.customerName) + "\n";
    billText += "Date: " + QString::fromStdString(o.date) + " " + QString::fromStdString(o.time) + "\n";
    billText += "----------------------------------------\n";
    billText += "Item               Qty    Price    Total\n";
    billText += "----------------------------------------\n";

    double grandTotal = 0;
    for (size_t i = 0; i < details.size(); ++i) {
        double lineTotal = (products[i].unitPrice + products[i].tax - products[i].discount) * details[i].quantity;
        grandTotal += lineTotal;
        
        QString name = QString::fromStdString(products[i].name).replace("_", " ");
        billText += name.leftJustified(18, ' ');
        billText += QString::number(details[i].quantity).leftJustified(6, ' ');
        billText += QString::number(products[i].unitPrice, 'f', 2).leftJustified(8, ' ');
        billText += QString::number(lineTotal, 'f', 2) + "\n";
    }

    billText += "----------------------------------------\n";
    billText += "Total Payable:              PKR " + QString::number(grandTotal, 'f', 2) + "\n";
    billText += "Payment Mode: " + QString::fromStdString(o.paymentMode) + "\n";
    billText += "========================================\n";
    billText += "           Thank you for shopping!      \n";

    textEdit->setText(billText);
    textEdit->setStyleSheet("font-family: 'Courier New', monospace; font-size: 14px; background-color: #fdfaf6; color: #333;");
    
    layout->addWidget(textEdit);
    
    QPushButton *closeBtn = new QPushButton("Close & Print");
    closeBtn->setStyleSheet("background-color: #4f46e5; color: white; padding: 10px; font-weight: bold; border-radius: 5px;");
    connect(closeBtn, &QPushButton::clicked, billDialog, &QDialog::accept);
    layout->addWidget(closeBtn);

    billDialog->exec();
}

void OrderRecordingScreen::refreshRecentOrders() {
    recentOrdersTable->setRowCount(0);
    auto orders = OrderManager::loadOrders();
    int numOrders = orders.size();
    
    int count = 0;
    for (int i = numOrders - 1; i >= 0 && count < 5; --i, ++count) {
        recentOrdersTable->insertRow(count);
        recentOrdersTable->setItem(count, 0, new QTableWidgetItem(QString::fromStdString(orders[i].orderId)));
        recentOrdersTable->setItem(count, 1, new QTableWidgetItem(QString::fromStdString(orders[i].customerName)));
        recentOrdersTable->setItem(count, 2, new QTableWidgetItem(QString::fromStdString(orders[i].status)));
        recentOrdersTable->setItem(count, 3, new QTableWidgetItem(QString::number(orders[i].paymentAmount, 'f', 2)));
    }
}

void OrderRecordingScreen::manualRefresh() {
    refreshRecentOrders();
    statusLabel->setText("Recent orders refreshed.");
}

void OrderRecordingScreen::clearScreen() {
    cartDetails.clear();
    cartProducts.clear();
    updateCartUI();
    
    custNameInput->clear();
    contactInput->clear();
    merchantInput->clear();

    std::ofstream f1("Order.txt", std::ios::trunc);
    std::ofstream f2("OrderDetail.txt", std::ios::trunc);
    std::ofstream f3("Bill.txt", std::ios::trunc);    
    refreshRecentOrders();
    statusLabel->setText("All data permanently cleared from files.");
}