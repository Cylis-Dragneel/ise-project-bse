#include "mainwindow.h"
#include "OrderRecordingScreen.h"
#include "../managers/OrderManager.h" // Include OrderManager for real stats
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Pazarla - Point of Sale");
    resize(1200, 800);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- SIDEBAR ---
    QWidget *sidebar = new QWidget();
    sidebar->setFixedWidth(250);
    sidebar->setObjectName("Sidebar");
    
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(20, 30, 20, 30);

    QLabel *logo = new QLabel("🛒 Pazarla");
    logo->setObjectName("Logo");
    QLabel *tagline = new QLabel("Smarter Scanning,\nFaster Selling.");
    tagline->setObjectName("Tagline");

    QPushButton *btnOrder = new QPushButton("Record Order");
    btnOrder->setObjectName("NavBtn");
    QPushButton *btnDashboard = new QPushButton("Dashboard");
    btnDashboard->setObjectName("NavBtn");
    
    sidebarLayout->addWidget(logo);
    sidebarLayout->addWidget(tagline);
    sidebarLayout->addSpacing(30);
    sidebarLayout->addWidget(btnOrder);
    sidebarLayout->addWidget(btnDashboard);
    sidebarLayout->addStretch();

    // --- MAIN CONTENT AREA ---
    QWidget *contentArea = new QWidget();
    contentArea->setObjectName("ContentArea");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(30, 30, 30, 30);

    QStackedWidget *stackedWidget = new QStackedWidget();
    
    // 1. Order Screen
    OrderRecordingScreen *orderScreen = new OrderRecordingScreen(this);
    orderScreen->setObjectName("CardWidget");
    
    // 2. Dynamic Dashboard Screen
    QWidget *dashboardScreen = new QWidget();
    dashboardScreen->setObjectName("CardWidget");
    QVBoxLayout *dashLayout = new QVBoxLayout(dashboardScreen);
    
    QLabel *dashTitle = new QLabel("Today's Sales Summary");
    dashTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #333;");
    
    QLabel *dashStats = new QLabel("Loading data...");
    dashStats->setStyleSheet("font-size: 16px; color: #555; line-height: 2;");
    
    dashLayout->addWidget(dashTitle);
    dashLayout->addWidget(dashStats);
    dashLayout->addStretch();

    stackedWidget->addWidget(orderScreen);
    stackedWidget->addWidget(dashboardScreen);

    contentLayout->addWidget(stackedWidget);
    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(contentArea);

    // Helper lambda to update Dashboard stats
    auto updateDashboard = [dashStats]() {
        auto orders = OrderManager::loadOrders();
        double totalRevenue = 0;
        int paidOrders = 0;
        
        for (const auto& o : orders) {
            if (o.status == "paid") {
                totalRevenue += o.paymentAmount;
                paidOrders++;
            }
        }
        
        QString text = QString("Total Orders Processed: %1\nPaid Orders: %2\nTotal Revenue: PKR %3")
                        .arg(orders.size())
                        .arg(paidOrders)
                        .arg(QString::number(totalRevenue, 'f', 2));
        dashStats->setText(text);
    };

    // Initial load
    updateDashboard();

    // Connect Buttons
    connect(btnOrder, &QPushButton::clicked, [=]() {
        stackedWidget->setCurrentIndex(0);
    });
    connect(btnDashboard, &QPushButton::clicked, [=]() {
        updateDashboard(); // Refresh stats when clicked
        stackedWidget->setCurrentIndex(1);
    });

    // Connect Order Saved Signal to refresh Dashboard automatically
    connect(orderScreen, &OrderRecordingScreen::orderSaved, this, [=]() {
        updateDashboard();
    });

    // Stylesheet
    setStyleSheet(R"(
        QWidget#Sidebar { background-color: #2b2e3b; }
        QLabel#Logo { color: #ffffff; font-size: 24px; font-weight: bold; }
        QLabel#Tagline { color: #8e92a4; font-size: 12px; font-style: italic; }
        QPushButton#NavBtn { background-color: transparent; color: #a0a3b5; text-align: left; padding: 12px; font-size: 14px; border-radius: 8px; }
        QPushButton#NavBtn:hover { background-color: #3d4152; color: #ffffff; }
        QWidget#ContentArea { background-color: #f3f4f6; }
        QWidget#CardWidget { background-color: #ffffff; border-radius: 12px; border: 1px solid #e5e7eb; }
        QLineEdit, QComboBox { background-color: #f9fafb; border: 1px solid #d1d5db; border-radius: 6px; padding: 8px; font-size: 14px; }
        QTableWidget { border: none; background-color: #ffffff; alternate-background-color: #f9fafb; }
        QHeaderView::section { background-color: #ffffff; color: #6b7280; font-weight: bold; border: none; border-bottom: 2px solid #e5e7eb; padding: 8px; }
        QPushButton { background-color: #4f46e5; color: white; border-radius: 6px; padding: 10px 15px; font-weight: bold; }
        QPushButton:hover { background-color: #4338ca; }
    )");
}