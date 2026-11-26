#include "PosWindow.h"

#include "InventoryDialog.h"
#include "OrdersDialog.h"
#include "ProductDescriptionDialog.h"
#include "ProductsDialog.h"
#include "ReportsDialog.h"
#include "SeasonsDialog.h"
#include "billing/BillCalculator.h"
#include "BillPdfWriter.h"
#include "billing/FileBillDetailRepository.h"
#include "billing/FileBillRepository.h"
#include "camera/CameraHandler.h"
#include "catalog/NumberFormat.h"
#include "catalogStockAdjuster.h"
#include "managers/OrderManager.h"
#include "orderService.h"
#include "scanning/BarcodeScannerService.h"
#include "util/DateUtils.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStackedWidget>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVideoWidget>

#include <algorithm>
#include <cmath>
#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace {

std::string defaultProductPath()
{
    const QString local = "data/Product.txt";
    if (QFileInfo::exists(local))
    {
        return local.toStdString();
    }
    const QString nextToApp =
        QCoreApplication::applicationDirPath() + "/data/Product.txt";
    if (QFileInfo::exists(nextToApp))
    {
        return nextToApp.toStdString();
    }
    return local.toStdString();
}

std::string defaultSeasonPath()
{
    const QString local = "data/Seasons.txt";
    if (QFileInfo::exists(local))
    {
        return local.toStdString();
    }
    const QString nextToApp =
        QCoreApplication::applicationDirPath() + "/data/Seasons.txt";
    if (QFileInfo::exists(nextToApp))
    {
        return nextToApp.toStdString();
    }
    return local.toStdString();
}

} // namespace

PosWindow::PosWindow(QWidget *parent)
    : QWidget(parent),
      productPath_(defaultProductPath()),
      orderPath_("Order.txt"),
      detailPath_("OrderDetail.txt"),
      billPath_("Bill.txt"),
      billDetailPath_("BillDetail.txt"),
      seasonPath_(defaultSeasonPath()),
      catalog_(std::make_shared<FileProductRepository>(productPath_)),
      categories_(std::make_shared<FileCategoryRepository>("PCategory.txt")),
      resolver_(catalog_),
      seasons_(std::make_shared<FileSeasonRepository>(seasonPath_)),
      billing_(catalog_, std::make_shared<FileBillRepository>(billPath_),
               std::make_shared<FileBillDetailRepository>(billDetailPath_),
               &seasons_, detailPath_)
{
    try
    {
        catalog_.load();
    }
    catch (const std::exception &error)
    {
        catalogError_ = error.what();
    }

    try
    {
        categories_.load();
    }
    catch (const std::exception &error)
    {
        setStatus(QString("PCategory.txt could not be loaded: %1").arg(error.what()));
    }

    try
    {
        seasons_.load();
    }
    catch (const std::exception &error)
    {
        setStatus(QString("Season data ignored: %1").arg(error.what()));
    }

    setObjectName("PosRoot");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    stack_ = new QStackedWidget(this);
    stack_->addWidget(createRolePage());
    stack_->addWidget(createPosPage());
    stack_->addWidget(createManagerPage());
    layout->addWidget(stack_);

    setStyleSheet(R"(
        QWidget#RolePage, QWidget#PosPage, QWidget#ManagerPage { background: #0b1110; color: #e8f0ed; }
        QWidget#RolePage QLabel, QWidget#PosPage QLabel, QWidget#ManagerPage QLabel { color: #e8f0ed; }
        QWidget#RolePage QPushButton { background: #147a55; color: #fff; border: 1px solid #24966b; border-radius: 12px; min-height: 54px; min-width: 270px; padding: 10px 22px; font-size: 15px; font-weight: 700; }
        QWidget#RolePage QPushButton:hover { background: #199568; }
        QWidget#PosPage QGroupBox, QWidget#ManagerPage QGroupBox { background: #111a17; color: #f3f8f5; border: 1px solid #263b33; border-radius: 12px; margin-top: 13px; padding: 15px 12px 11px; }
        QWidget#PosPage QGroupBox::title, QWidget#ManagerPage QGroupBox::title { subcontrol-origin: margin; left: 13px; padding: 0 6px; color: #9ad9bd; font-weight: 700; }
        QWidget#PosPage QTableWidget, QWidget#ManagerPage QTableWidget { background: #101815; alternate-background-color: #15211c; color: #e8f0ed; border: 1px solid #263b33; border-radius: 10px; gridline-color: #20342c; selection-background-color: #176b4b; selection-color: #fff; }
        QWidget#PosPage QHeaderView::section, QWidget#ManagerPage QHeaderView::section { background: #183b2d; color: #eafff3; font-weight: 700; border: 0; padding: 10px 7px; }
        QWidget#PosPage QLineEdit, QWidget#PosPage QComboBox, QWidget#PosPage QDoubleSpinBox { background: #0d1512; border: 1px solid #2b4238; border-radius: 8px; padding: 9px; color: #f4faf6; min-height: 20px; }
        QWidget#PosPage QLineEdit:focus, QWidget#PosPage QComboBox:focus, QWidget#PosPage QDoubleSpinBox:focus { border: 1px solid #35c58a; }
        QWidget#PosPage QTextEdit { background: #0d1512; border: 1px solid #263b33; border-radius: 9px; color: #e8f0ed; padding: 8px; }
        QWidget#PosPage QPushButton, QWidget#ManagerPage QPushButton { background: #1b6b4d; color: #fff; border: 1px solid #27845f; border-radius: 8px; padding: 10px 14px; min-height: 20px; font-weight: 650; }
        QWidget#PosPage QPushButton:hover, QWidget#ManagerPage QPushButton:hover { background: #25845f; }
        QWidget#PosPage QPushButton:disabled { background: #26332d; color: #75847c; border-color: #26332d; }
        QScrollArea { border: 0; background: #0b1110; }
    )");
}

PosWindow::~PosWindow()
{
    if (cameraHandler_ != nullptr)
    {
        cameraHandler_->stopCamera();
    }
}

QWidget *PosWindow::createRolePage()
{
    QWidget *page = new QWidget(this);
    page->setObjectName("RolePage");

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->addStretch();

    QLabel *title = new QLabel("Pazarla Point of Sale", page);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 30px; font-weight: 700; color: #ffffff;");

    QLabel *subtitle = new QLabel("Choose how this terminal is being used.", page);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("font-size: 15px; color: #bdbdbd;");

    QPushButton *cashier = new QPushButton("Cashier", page);
    QPushButton *manager = new QPushButton("Manager", page);
    cashier->setMinimumSize(280, 56);
    manager->setMinimumSize(280, 56);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(18);
    layout->addWidget(cashier, 0, Qt::AlignHCenter);
    layout->addWidget(manager, 0, Qt::AlignHCenter);
    layout->addStretch();

    connect(cashier, &QPushButton::clicked, this,
            [this]() { selectRole(Role::Cashier); });
    connect(manager, &QPushButton::clicked, this,
            [this]() { selectRole(Role::Manager); });

    return page;
}

QWidget *PosWindow::createPosPage()
{
    QWidget *page = new QWidget(this);
    page->setObjectName("PosPage");

    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(24, 20, 24, 20);
    pageLayout->setSpacing(12);

    QHBoxLayout *topBar = new QHBoxLayout();
    roleLabel_ = new QLabel("Cashier", page);
    roleLabel_->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
    QPushButton *switchRoleButton = new QPushButton("Switch Role", page);
    switchRoleButton->setMaximumWidth(120);
    topBar->addWidget(roleLabel_);
    topBar->addStretch();
    topBar->addWidget(switchRoleButton);
    pageLayout->addLayout(topBar);

    QHBoxLayout *mainRow = new QHBoxLayout();
    mainRow->setSpacing(18);

    QVBoxLayout *left = new QVBoxLayout();
    left->setSpacing(11);

    cameraPreview_ = new QVideoWidget(page);
    cameraPreview_->setMinimumSize(260, 180);
    cameraPreview_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cameraPreview_->hide();
    left->addWidget(cameraPreview_);

    cameraPlaceholder_ = new QLabel(
        "Camera off\n\nUse Start Camera or enter a barcode.", page);
    cameraPlaceholder_->setMinimumSize(260, 180);
    cameraPlaceholder_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cameraPlaceholder_->setAlignment(Qt::AlignCenter);
    cameraPlaceholder_->setStyleSheet(
        "background: #101010; color: #bdbdbd; border: 1px solid #303030; border-radius: 8px;"
        "font-size: 14px; padding: 12px;");
    left->addWidget(cameraPlaceholder_);

    QHBoxLayout *cameraButtons = new QHBoxLayout();
    cameraButton_ = new QPushButton("Start Camera", page);
    stopCameraButton_ = new QPushButton("Stop Camera", page);
    stopCameraButton_->setEnabled(false);
    cameraButtons->addWidget(cameraButton_);
    cameraButtons->addWidget(stopCameraButton_);
    left->addLayout(cameraButtons);

    QGroupBox *scanBox = new QGroupBox("Scan or Enter Barcode", page);
    QVBoxLayout *scanLayout = new QVBoxLayout(scanBox);

    barcodeInput_ = new QLineEdit(scanBox);
    barcodeInput_->setMaxLength(6);
    barcodeInput_->setPlaceholderText("6-digit barcode");
    barcodeInput_->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[1-9][0-9]{0,5}"), barcodeInput_));

    quantityInput_ = new QDoubleSpinBox(scanBox);
    quantityInput_->setRange(1.0, 1000000.0);
    quantityInput_->setDecimals(0);
    quantityInput_->setSingleStep(1.0);
    quantityInput_->setValue(1.0);
    quantityInput_->setPrefix("Qty ");

    QPushButton *addButton = new QPushButton("Add to Cart", scanBox);
    scanLayout->addWidget(barcodeInput_);
    scanLayout->addWidget(quantityInput_);
    scanLayout->addWidget(addButton);
    left->addWidget(scanBox);

    QHBoxLayout *cartButtons = new QHBoxLayout();
    selectedQuantityInput_ = new QDoubleSpinBox(page);
    selectedQuantityInput_->setRange(1.0, 1000000.0);
    selectedQuantityInput_->setDecimals(0);
    selectedQuantityInput_->setSingleStep(1.0);
    selectedQuantityInput_->setValue(1.0);
    selectedQuantityInput_->setPrefix("Qty ");
    QPushButton *setQuantityButton = new QPushButton("Set Selected Qty", page);
    QPushButton *removeButton = new QPushButton("Remove Selected", page);
    cartButtons->addWidget(selectedQuantityInput_);
    cartButtons->addWidget(setQuantityButton);
    cartButtons->addWidget(removeButton);
    left->addLayout(cartButtons);

    QGroupBox *specsBox = new QGroupBox("Product Specifications", page);
    QVBoxLayout *specsLayout = new QVBoxLayout(specsBox);
    specsView_ = new QTextEdit(specsBox);
    specsView_->setReadOnly(true);
    specsView_->setMinimumHeight(90);
    specsView_->setMaximumHeight(180);
    specsView_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    specsView_->setPlaceholderText("Scan or enter a barcode to see the product.");
    specsLayout->addWidget(specsView_);
    QPushButton *descriptionButton = new QPushButton("Description", specsBox);
    specsLayout->addWidget(descriptionButton);
    left->addWidget(specsBox);

    QVBoxLayout *right = new QVBoxLayout();
    right->setSpacing(11);

    cartTable_ = new QTableWidget(0, 7, page);
    cartTable_->setHorizontalHeaderLabels(
        {"Product", "Barcode", "Qty", "Unit Price", "Tax", "Discount", "Line Total"});
    cartTable_->horizontalHeader()->setStretchLastSection(true);
    cartTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    cartTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    cartTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    cartTable_->setAlternatingRowColors(true);
    cartTable_->verticalHeader()->setVisible(false);
    cartTable_->verticalHeader()->setDefaultSectionSize(34);
    cartTable_->horizontalHeader()->setMinimumSectionSize(62);
    right->addWidget(cartTable_, 1);

    totalsLabel_ = new QLabel(page);
    totalsLabel_->setStyleSheet(
        "font-size: 15px; font-weight: bold; color: #ffffff; padding: 4px;");
    right->addWidget(totalsLabel_);

    QGroupBox *customerBox = new QGroupBox("Customer & Payment", page);
    QFormLayout *customerForm = new QFormLayout(customerBox);
    customerInput_ = new QLineEdit(customerBox);
    customerInput_->setPlaceholderText("Customer name");
    contactInput_ = new QLineEdit(customerBox);
    contactInput_->setPlaceholderText("Contact (optional)");
    paymentModeInput_ = new QComboBox(customerBox);
    paymentModeInput_->addItems({"cash", "card", "wallet"});
    merchantInput_ = new QLineEdit(customerBox);
    merchantInput_->setPlaceholderText("Merchant (e.g. HBL)");
    customerForm->addRow("Customer", customerInput_);
    customerForm->addRow("Contact", contactInput_);
    customerForm->addRow("Payment", paymentModeInput_);
    customerForm->addRow("Merchant", merchantInput_);
    right->addWidget(customerBox);

    QHBoxLayout *orderActionRow = new QHBoxLayout();
    holdButton_ = new QPushButton("Hold Order", page);
    resumeButton_ = new QPushButton("Resume Order", page);
    QPushButton *openOrderButton = new QPushButton("Open Order", page);
    orderActionRow->addWidget(holdButton_);
    orderActionRow->addWidget(resumeButton_);
    orderActionRow->addWidget(openOrderButton);
    right->addLayout(orderActionRow);

    billButton_ = new QPushButton("Generate Bill", page);
    billButton_->setMinimumHeight(42);
    billButton_->setEnabled(false);
    right->addWidget(billButton_);

    mainRow->addLayout(left, 4);
    mainRow->addLayout(right, 6);
    pageLayout->addLayout(mainRow, 1);

    QHBoxLayout *recentHeader = new QHBoxLayout();
    QLabel *recentLabel = new QLabel("Recent Orders", page);
    recentLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #ffffff;");
    QPushButton *refreshButton = new QPushButton("Refresh", page);
    refreshButton->setMaximumWidth(100);
    recentHeader->addWidget(recentLabel);
    recentHeader->addStretch();
    recentHeader->addWidget(refreshButton);
    pageLayout->addLayout(recentHeader);

    recentTable_ = new QTableWidget(0, 4, page);
    recentTable_->setHorizontalHeaderLabels({"Order ID", "Customer", "Status", "Amount"});
    recentTable_->horizontalHeader()->setStretchLastSection(true);
    recentTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recentTable_->setMinimumHeight(90);
    recentTable_->setMaximumHeight(150);
    recentTable_->setAlternatingRowColors(true);
    recentTable_->verticalHeader()->setVisible(false);
    pageLayout->addWidget(recentTable_);

    managerTools_ = new QGroupBox("Manager Tools", page);
    managerTools_->setStyleSheet("QGroupBox { border: 1px solid #285a49; background: #0d1d18; } QGroupBox::title { color: #ffffff; }");
    QHBoxLayout *managerLayout = new QHBoxLayout(managerTools_);
    QPushButton *reloadButton = new QPushButton("Reload Product.txt", managerTools_);
    managerLayout->addWidget(reloadButton);
    managerLayout->addStretch();
    managerTools_->setVisible(false);
    pageLayout->addWidget(managerTools_);

    statusLabel_ = new QLabel("Ready.", page);
    statusLabel_->setStyleSheet("color: #bdbdbd;");
    pageLayout->addWidget(statusLabel_);

    cameraHandler_ = new CameraHandler(cameraPreview_, this);
    scanner_ = new BarcodeScannerService(this);

    connect(cameraHandler_, &CameraHandler::frameReady,
            scanner_, &BarcodeScannerService::processFrame);

    connect(cameraHandler_, &CameraHandler::errorOccurred,
            this, [this](const QString &message) {
                scanner_->stop();
                cameraPreview_->hide();
                cameraPlaceholder_->show();
                stopCameraButton_->setEnabled(false);
                cameraButton_->setText("Start Camera");
                setStatus(message);
            });

    connect(scanner_, &BarcodeScannerService::barcodeDetected,
            this, [this](const std::string &barcode) {
                // Use the quantity currently set in the spinbox
                addBarcode(barcode, quantityInput_->value());
                cameraButton_->setText("Scan Next Item");
                stopCameraButton_->setEnabled(true);
            });

    connect(switchRoleButton, &QPushButton::clicked, this, [this]() {
        stopCameraInternal();
        stack_->setCurrentIndex(0);
    });
    connect(cameraButton_, &QPushButton::clicked,
            this, &PosWindow::startOrArmCamera);
    connect(stopCameraButton_, &QPushButton::clicked,
            this, &PosWindow::stopCameraInternal);
    connect(addButton, &QPushButton::clicked,
            this, &PosWindow::addFromInput);
    connect(barcodeInput_, &QLineEdit::returnPressed,
            this, &PosWindow::addFromInput);
    connect(barcodeInput_, &QLineEdit::textChanged, this,
            [this](const QString &text) {
                if (text.size() != 6)
                {
                    return;
                }
                const ProductResolver::Result result =
                    resolver_.lookup(text.toStdString());
                if (result.found())
                {
                    configureQuantitySpin(quantityInput_, *result.product);
                }
            });
    connect(setQuantityButton, &QPushButton::clicked,
            this, &PosWindow::setSelectedQuantity);
    connect(removeButton, &QPushButton::clicked,
            this, &PosWindow::removeSelected);
    connect(descriptionButton, &QPushButton::clicked,
            this, &PosWindow::showDescription);
    connect(billButton_, &QPushButton::clicked,
            this, &PosWindow::generateBill);
    connect(holdButton_, &QPushButton::clicked,
            this, &PosWindow::holdCurrentOrder);
    connect(resumeButton_, &QPushButton::clicked,
            this, &PosWindow::resumeOrder);
    connect(openOrderButton, &QPushButton::clicked, this, [this]() {
        OrdersDialog dlg(orderPath_, this);
        if (dlg.exec() == QDialog::Accepted && !dlg.selectedOrderId().empty())
        {
            openOrderForEditing(dlg.selectedOrderId());
        }
    });
    connect(refreshButton, &QPushButton::clicked,
            this, &PosWindow::refreshRecentOrders);
    connect(reloadButton, &QPushButton::clicked, this, [this]() {
        try
        {
            catalog_.load();
            cart_.clear();
            updateCart();
            setStatus("Product.txt reloaded.");
        }
        catch (const std::exception &error)
        {
            setStatus(QString("Could not reload Product.txt: %1").arg(error.what()));
        }
    });
    connect(cartTable_, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = selectedRow();
        if (row < 0)
        {
            return;
        }
        selectedQuantityInput_->setValue(cart_[row].quantity);
        configureQuantitySpin(selectedQuantityInput_, cart_[row].product);
    });

    if (!catalogError_.empty())
    {
        setStatus(QString("Product.txt could not be loaded: %1")
                      .arg(QString::fromStdString(catalogError_)));
    }

    refreshRecentOrders();
    updateCart();

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidget(page);
    return scrollArea;
}

QWidget *PosWindow::createManagerPage()
{
    QWidget *page = new QWidget(this);
    page->setObjectName("ManagerPage");

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(18);

    // --- Header ---
    QHBoxLayout *headingRow = new QHBoxLayout();
    QVBoxLayout *headingText = new QVBoxLayout();
    QLabel *title = new QLabel("Pazarla", page);
    title->setStyleSheet("font-size: 30px; font-weight: 800; color: #ffffff;");
    QLabel *subtitle = new QLabel("MANAGER DASHBOARD", page);
    subtitle->setStyleSheet("font-size: 15px; color: #c9c9c9; letter-spacing: 1px;");
    headingText->addWidget(title);
    headingText->addWidget(subtitle);
    headingRow->addLayout(headingText);
    headingRow->addStretch();

    QLabel *systemBadge = new QLabel("POS SYSTEM", page);
    systemBadge->setStyleSheet("background: #063b22; color: #00c96b; border-radius: 15px; padding: 8px 16px; font-weight: 800;");
    headingRow->addWidget(systemBadge, 0, Qt::AlignTop);
    QPushButton *switchRole = new QPushButton("Switch Role", page);
    switchRole->setMaximumWidth(130);
    headingRow->addWidget(switchRole, 0, Qt::AlignTop);
    layout->addLayout(headingRow);

    // --- Cards ---
    QGridLayout *cards = new QGridLayout();
    cards->setHorizontalSpacing(16);
    cards->setVerticalSpacing(16);

    const auto makeCard = [page](const QString &name, const QString &description,
                                 const QString &color, const QString &hover) {
        QPushButton *card = new QPushButton(page);
        card->setMinimumHeight(190);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        card->setText(QString("%1\n%2").arg(name, description));
        card->setStyleSheet(QString(
            "QPushButton { "
            "  text-align: left; "
            "  white-space: pre-wrap; "
            "  background: %1; "
            "  color: white; "
            "  border: 1px solid #426b58; "
            "  border-radius: 16px; "
            "  padding: 30px 24px; "
            "  font-size: 18px; "
            "  font-weight: 600; "
            "} "
            "QPushButton:hover { background: %2; }")
            .arg(color, hover));
        return card;
    };

    QPushButton *products = makeCard("Products", "Manage products", "#183f37", "#225447");
    QPushButton *inventory = makeCard("Inventory", "Manage stock", "#28516d", "#356783");
    QPushButton *categories = makeCard("Categories", "Barcode management", "#79552f", "#90683b");
    QPushButton *reports = makeCard("Reports", "Sales analytics", "#59436f", "#6d5486");
    QPushButton *seasons = makeCard("Seasons", "Discount periods", "#5a4d7a", "#6f6195");
    QPushButton *orders = makeCard("Orders", "Open and edit orders", "#3f4f6d", "#52658a");

    cards->addWidget(products, 0, 0);
    cards->addWidget(inventory, 0, 1);
    cards->addWidget(categories, 1, 0);
    cards->addWidget(reports, 1, 1);
    cards->addWidget(seasons, 2, 0);
    cards->addWidget(orders, 2, 1);

    cards->setColumnStretch(0, 1);
    cards->setColumnStretch(1, 1);
    cards->setRowStretch(0, 1);
    cards->setRowStretch(1, 1);
    cards->setRowStretch(2, 1);
    layout->addLayout(cards, 1);

    // --- Footer ---
    QFrame *separator = new QFrame(page);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color: #292929;");
    layout->addWidget(separator);

    QHBoxLayout *bottomRow = new QHBoxLayout();
    QLabel *posLabel = new QLabel("Cashier / Point of Sale", page);
    posLabel->setStyleSheet("font-size: 19px; font-weight: 700; color: #eeeeee;");
    bottomRow->addWidget(posLabel);
    bottomRow->addStretch();
    QPushButton *openCashier = new QPushButton("Open Cashier", page);
    bottomRow->addWidget(openCashier);
    layout->addLayout(bottomRow);

    QLabel *managerStatus = new QLabel("Manager dashboard ready.", page);
    managerStatus->setStyleSheet("color: #a7a7a7; padding-top: 4px;");
    layout->addWidget(managerStatus);

    // --- Connections ---
    connect(switchRole, &QPushButton::clicked, this, [this]() {
        stopCameraInternal();
        stack_->setCurrentIndex(0);
    });

    connect(openCashier, &QPushButton::clicked, this, [this]() {
        role_ = Role::Cashier;
        roleLabel_->setText("Cashier");
        stack_->setCurrentIndex(1);
        setStatus("Cashier mode.");
    });

    connect(products, &QPushButton::clicked, this, [this]() {
        ProductsDialog dlg(&catalog_, &categories_, this);
        dlg.exec();
    });

    connect(inventory, &QPushButton::clicked, this, [this]() {
        InventoryDialog dlg(&catalog_, this);
        dlg.exec();
    });

    connect(categories, &QPushButton::clicked, this, [this]() {
        ProductsDialog dlg(&catalog_, &categories_, this);
        dlg.exec();
    });

    connect(reports, &QPushButton::clicked, this, [this]() {
        ReportsDialog dlg(catalog_, orderPath_, detailPath_,
                          billPath_, billDetailPath_, &seasons_, this);
        dlg.exec();
    });

    connect(seasons, &QPushButton::clicked, this, [this]() {
        SeasonsDialog dlg(&seasons_, this);
        dlg.exec();
    });

    connect(orders, &QPushButton::clicked, this, [this]() {
        OrdersDialog dlg(orderPath_, this);
        if (dlg.exec() == QDialog::Accepted && !dlg.selectedOrderId().empty())
        {
            openOrderForEditing(dlg.selectedOrderId());
        }
    });

    return page;
}

void PosWindow::selectRole(Role role)
{
    role_ = role;
    roleLabel_->setText(role == Role::Manager ? "Manager" : "Cashier");
    managerTools_->setVisible(role == Role::Manager);
    stack_->setCurrentIndex(role == Role::Manager ? 2 : 1);
    setStatus(role == Role::Manager ? "Manager mode." : "Cashier mode.");
}

void PosWindow::startOrArmCamera()
{
    if (cameraHandler_->isRunning())
    {
        scanner_->start();
        cameraButton_->setText("Scanning...");
        stopCameraButton_->setEnabled(true);
        setStatus("Camera running. Point it at a barcode.");
        return;
    }

    cameraPlaceholder_->hide();
    cameraPreview_->show();
    if (!cameraHandler_->startCamera())
    {
        cameraPreview_->hide();
        cameraPlaceholder_->show();
        cameraButton_->setText("Start Camera");
        setStatus("No camera available. Use the barcode field instead.");
        return;
    }

    scanner_->start();
    cameraButton_->setText("Scanning...");
    stopCameraButton_->setEnabled(true);
    setStatus("Camera running. Point it at a barcode.");
}

void PosWindow::stopCameraInternal()
{
    scanner_->stop();
    cameraHandler_->stopCamera();
    cameraPreview_->hide();
    cameraPlaceholder_->show();
    cameraButton_->setText("Start Camera");
    stopCameraButton_->setEnabled(false);
    setStatus("Camera stopped.");
}

void PosWindow::stopCamera()
{
    if (cameraHandler_ != nullptr)
    {
        stopCameraInternal();
    }
}

void PosWindow::addScannedBarcode(const QString &barcode)
{
    if (barcode.isEmpty())
    {
        return;
    }

    if (!editingExistingOrder_)
    {
        role_ = Role::Cashier;
        roleLabel_->setText("Cashier");
        managerTools_->setVisible(false);
    }
    stack_->setCurrentIndex(1);
    barcodeInput_->setText(barcode);

    // Respect the quantity spinbox here too
    addBarcode(barcode.toStdString(), quantityInput_->value());
}

bool PosWindow::addBarcode(const std::string &barcode, double quantity)
{
    if (!(quantity > 0) || !std::isfinite(quantity))
    {
        setStatus("Quantity must be greater than 0.");
        return false;
    }

    const ProductResolver::Result probe = resolver_.lookup(barcode);
    if (!probe.found())
    {
        setStatus(QString::fromStdString(probe.message));
        return false;
    }

    const Product &product = *probe.product;

    double existing = 0;
    int index = -1;
    for (std::size_t i = 0; i < cart_.size(); ++i)
    {
        if (cart_[i].product.id() == product.id())
        {
            existing = cart_[i].quantity;
            index = static_cast<int>(i);
            break;
        }
    }

    const double newTotal = existing + quantity;
    const ProductResolver::Result result =
        resolver_.lookupForQuantity(product.barcode(), newTotal);

    if (!result.found())
    {
        setStatus(QString::fromStdString(result.message));
        if (result.product != nullptr)
        {
            lastProduct_ = *result.product;
            specsView_->setPlainText(QString::fromStdString(result.description));
            configureQuantitySpin(quantityInput_, *result.product);
        }
        return false;
    }

    lastProduct_ = *result.product;
    specsView_->setPlainText(QString::fromStdString(result.description));
    configureQuantitySpin(quantityInput_, *result.product);

    if (index >= 0)
    {
        cart_[index].quantity = newTotal;
    }
    else
    {
        cart_.push_back(CartLine{*result.product, quantity});
    }

    updateCart();
    setStatus(QString("Added %1 x %2.")
                  .arg(QString::fromStdString(NumberFormat::general(quantity)))
                  .arg(QString::fromStdString(product.name())));
    return true;
}

void PosWindow::addFromInput()
{
    const std::string barcode = barcodeInput_->text().toStdString();
    if (barcode.empty())
    {
        setStatus("Enter a barcode first.");
        return;
    }

    if (addBarcode(barcode, quantityInput_->value()))
    {
        barcodeInput_->clear();
        barcodeInput_->setFocus();
        quantityInput_->setValue(1.0);
    }
}

void PosWindow::setSelectedQuantity()
{
    const int row = selectedRow();
    if (row < 0)
    {
        setStatus("Select a cart row first.");
        return;
    }

    const Product &product = cart_[row].product;
    const double quantity = selectedQuantityInput_->value();
    const ProductResolver::Result result =
        resolver_.lookupForQuantity(product.barcode(), quantity);

    if (!result.found())
    {
        setStatus(QString::fromStdString(result.message));
        return;
    }

    cart_[row].quantity = quantity;
    updateCart();
    setStatus("Quantity updated.");
}

void PosWindow::removeSelected()
{
    const int row = selectedRow();
    if (row < 0)
    {
        setStatus("Select a cart row first.");
        return;
    }

    cart_.erase(cart_.begin() + row);
    updateCart();
    setStatus("Item removed.");
}

void PosWindow::updateCart()
{
    cartTable_->setRowCount(0);

    for (std::size_t i = 0; i < cart_.size(); ++i)
    {
        const CartLine &line = cart_[i];

        BillLine billLine;
        billLine.quantity = line.quantity;
        billLine.unitPrice = line.product.unitPrice();
        billLine.taxTotal = BillCalculator::roundMoney(
            line.product.taxAmount() * line.quantity);
        billLine.discountTotal = BillCalculator::roundMoney(
            line.product.discountPerUnit() * line.quantity);

        const int row = static_cast<int>(i);
        cartTable_->insertRow(row);
        cartTable_->setItem(row, 0,
            new QTableWidgetItem(QString::fromStdString(line.product.name())));
        cartTable_->setItem(row, 1,
            new QTableWidgetItem(QString::fromStdString(line.product.barcode())));
        cartTable_->setItem(row, 2,
            new QTableWidgetItem(QString::fromStdString(NumberFormat::general(line.quantity))));
        cartTable_->setItem(row, 3,
            new QTableWidgetItem(QString::fromStdString(NumberFormat::money(line.product.unitPrice()))));
        cartTable_->setItem(row, 4,
            new QTableWidgetItem(QString::fromStdString(NumberFormat::money(line.product.taxAmount()))));
        cartTable_->setItem(row, 5,
            new QTableWidgetItem(QString::fromStdString(NumberFormat::money(line.product.discountPerUnit()))));
        cartTable_->setItem(row, 6,
            new QTableWidgetItem(QString::fromStdString(NumberFormat::money(BillCalculator::lineTotal(billLine)))));
    }

    billButton_->setEnabled(!cart_.empty());

    if (cart_.empty())
    {
        selectedQuantityInput_->setDecimals(0);
        selectedQuantityInput_->setRange(1.0, 1000000.0);
        selectedQuantityInput_->setValue(1.0);
    }

    updateTotals();
}

void PosWindow::updateTotals()
{
    if (cart_.empty())
    {
        totalsLabel_->setText("Cart is empty.");
        return;
    }

    std::vector<OrderDetail> details;
    details.reserve(cart_.size());
    for (const CartLine &line : cart_)
    {
        OrderDetail d;
        d.productId = line.product.id();
        d.productBarcode = line.product.barcode();
        d.quantity = line.quantity;
        details.push_back(d);
    }

    Order probe;
    probe.orderId = "00000000";
    probe.date = DateUtils::today();
    probe.time = DateUtils::nowTime();
    probe.status = "in-progress";

    try
    {
        const Bill quote = billing_.quote(probe, details, 0.0, 0.0, "", "",
                                          !editingExistingOrder_);
        totalsLabel_->setText(
            QString("Subtotal %1 | Tax %2 | Discount %3 | Seasonal %4 | Total %5")
                .arg(QString::fromStdString(NumberFormat::money(quote.totalNoTax)))
                .arg(QString::fromStdString(NumberFormat::money(quote.totalTax)))
                .arg(QString::fromStdString(NumberFormat::money(quote.totalDiscount)))
                .arg(QString::fromStdString(NumberFormat::money(quote.seasonalDiscount)))
                .arg(QString::fromStdString(NumberFormat::money(quote.totalPayable))));
    }
    catch (const std::exception &e)
    {
        totalsLabel_->setText(QString("Cannot compute totals: %1").arg(e.what()));
    }
}

void PosWindow::showDescription()
{
    const int row = selectedRow();
    if (row >= 0)
    {
        ProductDescriptionDialog dialog(cart_[row].product, this);
        dialog.exec();
        return;
    }

    if (lastProduct_)
    {
        ProductDescriptionDialog dialog(*lastProduct_, this);
        dialog.exec();
        return;
    }

    setStatus("Scan or select a product first.");
}

int PosWindow::selectedRow() const
{
    return cartTable_->currentRow();
}

std::vector<OrderDetail> PosWindow::cartToDetails(const std::string &orderId) const
{
    std::vector<OrderDetail> details;
    details.reserve(cart_.size());
    for (const CartLine &line : cart_)
    {
        OrderDetail d;
        d.orderId = orderId;
        d.productId = line.product.id();
        d.productBarcode = line.product.barcode();
        d.quantity = line.quantity;
        details.push_back(d);
    }
    return details;
}

void PosWindow::holdCurrentOrder()
{
    if (cart_.empty())
    {
        setStatus("Cart is empty - nothing to hold.");
        return;
    }

    Order order;
    order.date = OrderManager::getCurrentDate();
    order.time = OrderManager::getCurrentTime();
    order.orderId = OrderManager::generateOrderId(order.date, orderPath_);
    if (order.orderId.empty())
    {
        setStatus("Could not allocate an order id.");
        return;
    }
    order.customerName = customerInput_->text().trimmed().toStdString();
    if (order.customerName.empty())
    {
        order.customerName = "Hold";
    }
    order.contact = contactInput_->text().trimmed().toStdString();
    order.paymentMode = paymentModeInput_->currentText().toStdString();
    order.merchant = merchantInput_->text().trimmed().toStdString();
    order.status = "on-hold";

    CatalogStockAdjuster adjuster(catalog_);
    const OrderPaths paths{orderPath_, detailPath_};

    const std::vector<OrderDetail> details = cartToDetails(order.orderId);
    const OrderResult r = OrderService::holdOrder(order, details, adjuster, paths);
    if (r != OrderResult::Ok)
    {
        setStatus(QString::fromStdString(describe(r)));
        return;
    }

    cart_.clear();
    lastProduct_.reset();
    customerInput_->clear();
    contactInput_->clear();
    merchantInput_->clear();
    updateCart();
    refreshRecentOrders();
    setStatus(QString("Order %1 held.")
                  .arg(QString::fromStdString(order.orderId)));
}

void PosWindow::resumeOrder()
{
    const std::vector<Order> held =
        OrderManager::loadOrdersByStatus("on-hold", orderPath_);

    if (held.empty())
    {
        setStatus("No on-hold orders to resume.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Resume Held Order");
    dlg.resize(500, 320);
    auto *lay = new QVBoxLayout(&dlg);
    auto *table = new QTableWidget(static_cast<int>(held.size()), 3, &dlg);
    table->setHorizontalHeaderLabels({"Order ID", "Customer", "Date"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    for (std::size_t i = 0; i < held.size(); ++i)
    {
        const int row = static_cast<int>(i);
        table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(held[i].orderId)));
        table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(held[i].customerName)));
        table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(held[i].date)));
    }
    table->selectRow(0);
    lay->addWidget(table, 1);
    auto *buttons = new QHBoxLayout();
    auto *ok = new QPushButton("Resume", &dlg);
    auto *cancel = new QPushButton("Cancel", &dlg);
    buttons->addStretch();
    buttons->addWidget(ok);
    buttons->addWidget(cancel);
    lay->addLayout(buttons);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(table, &QTableWidget::cellDoubleClicked, &dlg,
            [&dlg](int, int) { dlg.accept(); });

    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }
    const int row = table->currentRow();
    if (row < 0)
    {
        return;
    }

    Order loaded;
    std::vector<OrderDetail> details;
    const OrderResult r = OrderService::resumeOrder(
        held[static_cast<std::size_t>(row)].orderId,
        loaded, details, {orderPath_, detailPath_});
    if (r != OrderResult::Ok)
    {
        setStatus(QString::fromStdString(describe(r)));
        return;
    }

    cart_.clear();
    for (const OrderDetail &d : details)
    {
        const Product *p = catalog_.findByBarcode(d.productBarcode);
        if (p != nullptr)
        {
            cart_.push_back(CartLine{*p, d.quantity});
        }
    }
    activeOrder_ = loaded;
    editingExistingOrder_ = true;
    managerApproved_ = false;
    customerInput_->setText(QString::fromStdString(loaded.customerName));
    contactInput_->setText(QString::fromStdString(loaded.contact));
    merchantInput_->setText(QString::fromStdString(loaded.merchant));
    updateCart();
    setStatus(QString("Resumed order %1.")
                  .arg(QString::fromStdString(loaded.orderId)));
}

void PosWindow::openOrderForEditing(const std::string &orderId)
{
    if (!cart_.empty() &&
        (!editingExistingOrder_ || activeOrder_.orderId != orderId))
    {
        QMessageBox msg(this);
        msg.setIcon(QMessageBox::Question);
        msg.setWindowTitle("Open Order");
        msg.setText("Replace the current cart with this order?");
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");

        if (msg.exec() != QMessageBox::Yes)
        {
            return;
        }
    }

    Order loaded;
    if (!OrderManager::findOrder(orderId, loaded, orderPath_))
    {
        setStatus(QString("Order %1 was not found.")
                      .arg(QString::fromStdString(orderId)));
        return;
    }

    const std::vector<OrderDetail> details =
        OrderManager::loadOrderDetails(orderId, detailPath_);
    if (details.empty())
    {
        setStatus("That order has no items.");
        return;
    }

    std::vector<CartLine> loadedCart;
    for (const OrderDetail &d : details)
    {
        const Product *p = catalog_.findByBarcode(d.productBarcode);
        if (p == nullptr)
        {
            p = catalog_.findById(d.productId);
        }
        if (p != nullptr)
        {
            loadedCart.push_back(CartLine{*p, d.quantity});
        }
    }

    if (loadedCart.empty())
    {
        setStatus("None of that order's products are in Product.txt.");
        return;
    }

    cart_ = loadedCart;
    activeOrder_ = loaded;
    editingExistingOrder_ = true;
    managerApproved_ = false;

    customerInput_->setText(QString::fromStdString(loaded.customerName));
    contactInput_->setText(QString::fromStdString(loaded.contact));
    merchantInput_->setText(QString::fromStdString(loaded.merchant));
    const int modeIndex = paymentModeInput_->findText(
        QString::fromStdString(loaded.paymentMode));
    if (modeIndex >= 0)
    {
        paymentModeInput_->setCurrentIndex(modeIndex);
    }

    managerTools_->setVisible(role_ == Role::Manager);
    stack_->setCurrentIndex(1);
    updateCart();
    setStatus(QString("Editing order %1 (%2).")
                  .arg(QString::fromStdString(loaded.orderId),
                       QString::fromStdString(loaded.status)));
}

void PosWindow::generateBill()
{
    if (cart_.empty())
    {
        QMessageBox msg(this);
        msg.setIcon(QMessageBox::Warning);
        msg.setWindowTitle("Generate Bill");
        msg.setText("The cart is empty.");
        msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");
        msg.exec();
        return;
    }

    if (customerInput_->text().trimmed().isEmpty())
    {
        QMessageBox msg(this);
        msg.setIcon(QMessageBox::Warning);
        msg.setWindowTitle("Generate Bill");
        msg.setText("Enter a customer name.");
        msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");
        msg.exec();
        return;
    }

    const std::string customer = customerInput_->text().trimmed().toStdString();
    const std::string contact = contactInput_->text().trimmed().toStdString();
    const std::string mode = paymentModeInput_->currentText().toStdString();
    const std::string merchant = merchantInput_->text().trimmed().toStdString();

    CatalogStockAdjuster adjuster(catalog_);
    const OrderPaths paths{orderPath_, detailPath_};

    Order order;
    if (editingExistingOrder_)
    {
        order = activeOrder_;
        order.customerName = customer;
        order.contact = contact;
        order.paymentMode = mode;
        order.merchant = merchant;
        order.status = "paid";
    }
    else
    {
        order.date = OrderManager::getCurrentDate();
        order.time = OrderManager::getCurrentTime();
        order.orderId = OrderManager::generateOrderId(order.date, orderPath_);
        if (order.orderId.empty())
        {
            QMessageBox msg(this);
            msg.setIcon(QMessageBox::Critical);
            msg.setWindowTitle("Generate Bill");
            msg.setText("Could not allocate an order id.");
            msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");
            msg.exec();
            return;
        }
        order.customerName = customer;
        order.contact = contact;
        order.paymentMode = mode;
        order.merchant = merchant;
        order.status = "paid";
    }

    std::vector<OrderDetail> details = cartToDetails(order.orderId);

    Bill quote;
    try
    {
        quote = billing_.quote(order, details, 0.0, 0.0, "", "",
                               !editingExistingOrder_);
        order.paymentAmount = quote.totalPayable;
    }
    catch (const std::exception &e)
    {
        QMessageBox msg(this);
        msg.setIcon(QMessageBox::Critical);
        msg.setWindowTitle("Generate Bill");
        msg.setText(e.what());
        msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");
        msg.exec();
        return;
    }

    OrderResult result;
    BillStamp stamp;
    Bill existingBill;
    if (editingExistingOrder_ &&
        billing_.findBillByOrderId(order.orderId, existingBill))
    {
        stamp.hasBill = true;
        stamp.date = existingBill.billDate;
        stamp.time = existingBill.billTime;
    }

    if (editingExistingOrder_)
    {
        result = OrderService::editOrder(order, details, stamp,
                                         DateUtils::today(),
                                         managerApproved_, adjuster, paths);
        if (result == OrderResult::NeedsApproval)
        {
            if (role_ != Role::Manager)
            {
                QMessageBox msg(this);
                msg.setIcon(QMessageBox::Warning);
                msg.setWindowTitle("Manager Approval");
                msg.setText("This paid order is inside the 7-day edit window and needs "
                            "manager approval.\nSwitch to the Manager role and open the "
                            "order again to edit it.");
                msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");
                msg.exec();
                return;
            }
            QMessageBox msg(this);
            msg.setIcon(QMessageBox::Question);
            msg.setWindowTitle("Manager Approval");
            msg.setText("This is a paid order inside the 7-day edit window.\n"
                        "Approve this edit?");
            msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");

            if (msg.exec() == QMessageBox::Yes)
            {
                managerApproved_ = true;
                result = OrderService::editOrder(order, details, stamp,
                                                 DateUtils::today(),
                                                 true, adjuster, paths);
            }
            else
            {
                return;
            }
        }
    }
    else
    {
        result = OrderService::saveNewOrder(order, details, adjuster, paths);
    }

    if (result != OrderResult::Ok)
    {
        QMessageBox msg(this);
        msg.setIcon(QMessageBox::Critical);
        msg.setWindowTitle("Generate Bill");
        msg.setText(QString::fromStdString(describe(result)));
        msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");
        msg.exec();
        return;
    }

    try
    {
        Bill bill;
        if (editingExistingOrder_ && !existingBill.billId.empty())
        {
            bill = billing_.updateExistingBill(existingBill.billId,
                                               order, details,
                                               0.0, quote.totalPayable);
        }
        else
        {
            bill = billing_.generateBill(order, details, 0.0,
                                         quote.totalPayable, "", "", false);
        }
        showBill(order, bill, cart_);
    }
    catch (const std::exception &e)
    {
        QMessageBox msg(this);
        msg.setIcon(QMessageBox::Critical);
        msg.setWindowTitle("Generate Bill");
        msg.setText(e.what());
        msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");
        msg.exec();
        return;
    }

    cart_.clear();
    lastProduct_.reset();
    customerInput_->clear();
    contactInput_->clear();
    merchantInput_->clear();
    editingExistingOrder_ = false;
    managerApproved_ = false;
    activeOrder_ = Order();
    updateCart();
    refreshRecentOrders();
    emit orderSaved();
    setStatus("Order saved, stock updated, bill generated.");
}

void PosWindow::showBill(const Order &order, const Bill &bill,
                         const std::vector<CartLine> &lines)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Bill " + QString::fromStdString(bill.billId));
    dialog.resize(560, 680);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTextEdit *text = new QTextEdit(&dialog);
    text->setReadOnly(true);
    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    QString receipt;
    receipt += "========================================\n";
    receipt += "              PAZARLA POS               \n";
    receipt += "========================================\n";
    receipt += "Bill ID : " + QString::fromStdString(bill.billId) + "\n";
    receipt += "Order ID: " + QString::fromStdString(order.orderId) + "\n";
    receipt += "Customer: " + QString::fromStdString(order.customerName) + "\n";
    receipt += "Date    : " + QString::fromStdString(bill.billDate) + " " +
               QString::fromStdString(bill.billTime) + "\n";
    receipt += "----------------------------------------\n";
    receipt += "Item                 Qty    Price     Total\n";
    receipt += "----------------------------------------\n";

    for (std::size_t i = 0; i < bill.lines.size(); ++i)
    {
        const BillLine &line = bill.lines[i];
        const QString name =
            i < lines.size()
                ? QString::fromStdString(lines[i].product.name())
                : QString::fromStdString(line.productAcronym);

        receipt += name.leftJustified(20, ' ');
        receipt += QString::fromStdString(NumberFormat::general(line.quantity))
                       .leftJustified(7, ' ');
        receipt += QString::fromStdString(NumberFormat::money(line.unitPrice))
                       .leftJustified(10, ' ');
        receipt += QString::fromStdString(NumberFormat::money(line.lineTotal)) + "\n";
    }

    receipt += "----------------------------------------\n";
    receipt += "Subtotal (no tax) : PKR " +
               QString::fromStdString(NumberFormat::money(bill.totalNoTax)) + "\n";
    receipt += "Tax               : PKR " +
               QString::fromStdString(NumberFormat::money(bill.totalTax)) + "\n";
    receipt += "Discount          : PKR " +
               QString::fromStdString(NumberFormat::money(bill.totalDiscount)) + "\n";
    receipt += "Seasonal discount : PKR " +
               QString::fromStdString(NumberFormat::money(bill.seasonalDiscount)) + "\n";
    receipt += "Special discount  : " +
               QString::fromStdString(NumberFormat::general(bill.specialDiscountPercent)) +
               " %\n";
    receipt += "Total payable     : PKR " +
               QString::fromStdString(NumberFormat::money(bill.totalPayable)) + "\n";
    receipt += "Amount received   : PKR " +
               QString::fromStdString(NumberFormat::money(bill.amountReceived)) + "\n";
    receipt += "Balance returned  : PKR " +
               QString::fromStdString(NumberFormat::money(bill.balanceReturned)) + "\n";
    receipt += "Payment mode      : " + QString::fromStdString(order.paymentMode) + "\n";
    receipt += "Status            : " + QString::fromStdString(bill.paymentStatus) + "\n";
    receipt += "========================================\n";

    text->setPlainText(receipt);
    layout->addWidget(text);

    auto *buttons = new QHBoxLayout();
    auto *pdfButton = new QPushButton("Save PDF", &dialog);
    auto *closeButton = new QPushButton("Close", &dialog);
    buttons->addWidget(pdfButton);
    buttons->addWidget(closeButton);
    layout->addLayout(buttons);

    connect(pdfButton, &QPushButton::clicked, &dialog, [&dialog, bill]() {
        const QString path = QFileDialog::getSaveFileName(
            &dialog, "Save Bill as PDF",
            BillPdfWriter::defaultFileName(bill),
            "PDF files (*.pdf)");
        if (path.isEmpty())
        {
            return;
        }
        QString error;
        if (!BillPdfWriter::writeBillPdf(bill, path, &error))
        {
            QMessageBox msg(&dialog);
            msg.setIcon(QMessageBox::Warning);
            msg.setWindowTitle("PDF Error");
            msg.setText(error);
            msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");
            msg.exec();
        }
        else
        {
            QMessageBox msg(&dialog);
            msg.setIcon(QMessageBox::Information);
            msg.setWindowTitle("PDF Saved");
            msg.setText("Saved to:\n" + path);
            msg.setStyleSheet("QMessageBox { background-color: #0b1110; color: #ffffff; } QLabel { color: #ffffff; } QPushButton { background-color: #1b6b4d; color: #ffffff; padding: 4px 12px; }");
            msg.exec();
        }
    });
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

void PosWindow::refreshRecentOrders()
{
    recentTable_->setRowCount(0);

    const std::vector<Order> orders = OrderManager::loadOrders(orderPath_);
    int count = 0;
    for (int i = static_cast<int>(orders.size()) - 1; i >= 0 && count < 5;
         --i, ++count)
    {
        const Order &order = orders[static_cast<std::size_t>(i)];
        recentTable_->insertRow(count);
        recentTable_->setItem(count, 0,
            new QTableWidgetItem(QString::fromStdString(order.orderId)));
        recentTable_->setItem(count, 1,
            new QTableWidgetItem(QString::fromStdString(order.customerName)));
        recentTable_->setItem(count, 2,
            new QTableWidgetItem(QString::fromStdString(order.status)));
        recentTable_->setItem(count, 3,
            new QTableWidgetItem(QString::fromStdString(NumberFormat::money(order.paymentAmount))));
    }
}

void PosWindow::setStatus(const QString &text)
{
    if (statusLabel_ != nullptr)
    {
        statusLabel_->setText(text);
    }
}

void PosWindow::configureQuantitySpin(QDoubleSpinBox *spin, const Product &product)
{
    if (product.unit() == MeasurementUnit::Count)
    {
        spin->setDecimals(0);
        spin->setRange(1.0, 1000000.0);
        spin->setSingleStep(1.0);
        spin->setValue(std::max(1.0, std::round(spin->value())));
    }
    else
    {
        spin->setDecimals(3);
        spin->setRange(0.001, 1000000.0);
        spin->setSingleStep(0.1);
    }
}