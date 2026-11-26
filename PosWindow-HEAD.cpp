#include "PosWindow.h"

#include "ProductDescriptionDialog.h"
#include "billing/BillCalculator.h"
#include "billing/FileBillDetailRepository.h"
#include "billing/FileBillRepository.h"
#include "camera/CameraHandler.h"
#include "catalog/NumberFormat.h"
#include "managers/OrderManager.h"
#include "scanning/BarcodeScannerService.h"
#include "util/DateUtils.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDoubleSpinBox>
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

// Prefer data/ next to the working directory (the repo root / just run), then
// the copy CMake places next to the executable.
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

// Seasons are optional: a missing Seasons.txt simply means "no seasons yet".
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
        QWidget#RolePage { background: #000000; }
        QWidget#RolePage QLabel { color: #ffffff; }
        QWidget#RolePage QPushButton { background: #183f37; color: #ffffff; border: 1px solid #286052; border-radius: 10px; min-height: 56px; min-width: 280px; padding: 10px 22px; font-size: 16px; font-weight: 700; }
        QWidget#RolePage QPushButton:hover { background: #24594d; }
        QWidget#PosPage { background: #000000; color: #f2f2f2; }
        QWidget#PosPage QLabel { color: #eeeeee; }
        QWidget#PosPage QGroupBox { background: #090909; color: #ffffff; border: 1px solid #303030; border-radius: 10px; margin-top: 12px; padding: 14px 10px 10px; }
        QWidget#PosPage QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 5px; font-weight: 700; color: #ffffff; }
        QWidget#PosPage QTableWidget { background: #101010; alternate-background-color: #191919; color: #eeeeee; border: 1px solid #303030; border-radius: 8px; gridline-color: #292929; selection-background-color: #28516d; selection-color: #ffffff; }
        QWidget#PosPage QHeaderView::section { background: #183f37; color: #ffffff; font-weight: 700; border: 0; padding: 9px 6px; }
        QWidget#PosPage QLineEdit, QWidget#PosPage QComboBox, QWidget#PosPage QDoubleSpinBox { background: #151515; border: 1px solid #3b3b3b; border-radius: 7px; padding: 8px; color: #ffffff; }
        QWidget#PosPage QLineEdit:focus, QWidget#PosPage QComboBox:focus, QWidget#PosPage QDoubleSpinBox:focus { border: 1px solid #00a65a; }
        QWidget#PosPage QTextEdit { background: #101010; border: 1px solid #303030; border-radius: 8px; color: #eeeeee; padding: 7px; }
        QWidget#PosPage QPushButton { background: #28516d; color: #ffffff; border: 1px solid #356783; border-radius: 7px; padding: 9px 12px; min-height: 20px; font-weight: 600; }
        QWidget#PosPage QPushButton:hover { background: #356783; }
        QWidget#PosPage QPushButton:disabled { background: #292929; color: #777777; border-color: #292929; }
        QScrollArea { border: 0; background: #000000; }
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
    pageLayout->setContentsMargins(20, 18, 20, 18);
    pageLayout->setSpacing(12);

    // --- top bar -----------------------------------------------------------
    QHBoxLayout *topBar = new QHBoxLayout();
    roleLabel_ = new QLabel("Cashier", page);
    roleLabel_->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
    QPushButton *switchRoleButton = new QPushButton("Switch Role", page);
    switchRoleButton->setMaximumWidth(120);
    topBar->addWidget(roleLabel_);
    topBar->addStretch();
    topBar->addWidget(switchRoleButton);
    pageLayout->addLayout(topBar);

    // --- main row ----------------------------------------------------------
    QHBoxLayout *mainRow = new QHBoxLayout();
    mainRow->setSpacing(14);

    // Left: camera, scan controls, specs.
    QVBoxLayout *left = new QVBoxLayout();
    left->setSpacing(8);

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

    // Right: cart, totals, customer, bill.
    QVBoxLayout *right = new QVBoxLayout();
    right->setSpacing(8);

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

    billButton_ = new QPushButton("Generate Bill", page);
    billButton_->setMinimumHeight(42);
    billButton_->setEnabled(false);
    right->addWidget(billButton_);

    mainRow->addLayout(left, 4);
    mainRow->addLayout(right, 6);
    pageLayout->addLayout(mainRow, 1);

    // --- recent orders -----------------------------------------------------
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

    // --- manager-only tools ------------------------------------------------
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

    // --- camera ------------------------------------------------------------
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
                addBarcode(barcode, 1.0);
                // The service stops itself after one success; the button
                // re-arms it for the next item.
                cameraButton_->setText("Scan Next Item");
                stopCameraButton_->setEnabled(true);
            });

    // --- connections -------------------------------------------------------
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
                // Adapt the quantity field as soon as the barcode is complete,
                // so a weight product can take 1.5 and a count product cannot.
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

    // A scroll area prevents the lower controls from being clipped on
    // shorter displays while keeping the original widget hierarchy intact.
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
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(18);

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

    QGridLayout *cards = new QGridLayout();
    cards->setHorizontalSpacing(16);
    cards->setVerticalSpacing(16);
    const auto makeCard = [page](const QString &name, const QString &description,
                                 const QString &color, const QString &hover) {
        QPushButton *card = new QPushButton(page);
        card->setMinimumHeight(150);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        card->setText(QString("%1\n\n%2").arg(name, description));
        card->setStyleSheet(QString(
            "QPushButton { text-align: left; white-space: pre-wrap; background: %1; color: white; border: 0; border-radius: 16px; padding: 22px; font-size: 19px; font-weight: 700; }"
            "QPushButton:hover { background: %2; }").arg(color, hover));
        return card;
    };

    QPushButton *products = makeCard("⬡  Products", "Manage products", "#183f37", "#225447");
    QPushButton *inventory = makeCard("▤  Inventory", "Manage stock", "#28516d", "#356783");
    QPushButton *categories = makeCard("▥  Categories", "Barcode management", "#79552f", "#90683b");
    QPushButton *reports = makeCard("▰  Reports", "Sales analytics", "#59436f", "#6d5486");
    cards->addWidget(products, 0, 0);
    cards->addWidget(inventory, 0, 1);
    cards->addWidget(categories, 1, 0);
    cards->addWidget(reports, 1, 1);
    cards->setColumnStretch(0, 1);
    cards->setColumnStretch(1, 1);
    cards->setRowStretch(0, 1);
    cards->setRowStretch(1, 1);
    layout->addLayout(cards, 1);

    QFrame *separator = new QFrame(page);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color: #292929;");
    layout->addWidget(separator);

    QHBoxLayout *bottomRow = new QHBoxLayout();
    QLabel *posLabel = new QLabel("🛒  Cashier / Point of Sale", page);
    posLabel->setStyleSheet("font-size: 19px; font-weight: 700; color: #eeeeee;");
    bottomRow->addWidget(posLabel);
    bottomRow->addStretch();
    QPushButton *openCashier = new QPushButton("Open Cashier", page);
    bottomRow->addWidget(openCashier);
    layout->addLayout(bottomRow);

    QLabel *managerStatus = new QLabel("Manager dashboard ready.", page);
    managerStatus->setStyleSheet("color: #a7a7a7; padding-top: 4px;");
    layout->addWidget(managerStatus);

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
    connect(products, &QPushButton::clicked, this, [this, managerStatus]() {
        try {
            catalog_.load();
            managerStatus->setText("Product catalogue reloaded successfully.");
        } catch (const std::exception &error) {
            managerStatus->setText(QString("Could not reload products: %1").arg(error.what()));
        }
    });
    connect(inventory, &QPushButton::clicked, this, [managerStatus]() {
        managerStatus->setText("Inventory section selected.");
    });
    connect(categories, &QPushButton::clicked, this, [managerStatus]() {
        managerStatus->setText("Categories section selected.");
    });
    connect(reports, &QPushButton::clicked, this, [this, managerStatus]() {
        refreshRecentOrders();
        managerStatus->setText("Reports selected. Recent sales have been refreshed.");
    });

    page->setStyleSheet(R"(
        QWidget#ManagerPage { background: #000000; }
        QWidget#ManagerPage QLabel { color: #ffffff; }
        QWidget#ManagerPage QPushButton { background: #28516d; color: #ffffff; border: 0; border-radius: 8px; padding: 10px 16px; font-weight: 700; }
        QWidget#ManagerPage QPushButton:hover { background: #356783; }
    )");
    return page;
}

void PosWindow::selectRole(Role role)
{
    role_ = role;
    roleLabel_->setText(role == Role::Manager ? "Manager" : "Cashier");
    managerTools_->setVisible(false);
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
        // Do not leave the last selected row's quantity on screen.
        selectedQuantityInput_->setDecimals(0);
        selectedQuantityInput_->setRange(1.0, 1000000.0);
        selectedQuantityInput_->setValue(1.0);
    }

    updateTotals();
}

void PosWindow::updateTotals()
{
    Bill bill;
    for (const CartLine &line : cart_)
    {
        OrderDetail detail;
        detail.productId = line.product.id();
        detail.productBarcode = line.product.barcode();
        detail.quantity = line.quantity;
        bill.lines.push_back(BillCalculator::makeLine("", detail, line.product));
    }

    double seasonalPercent = 0;
    try
    {
        seasonalPercent = seasons_.activePercent(DateUtils::today());
    }
    catch (const std::exception &)
    {
        seasonalPercent = 0;
    }
    BillCalculator::calculate(bill, seasonalPercent);

    totalsLabel_->setText(
        QString("Subtotal %1 | Tax %2 | Discount %3 | Total %4")
            .arg(QString::fromStdString(NumberFormat::money(bill.totalNoTax)))
            .arg(QString::fromStdString(NumberFormat::money(bill.totalTax)))
            .arg(QString::fromStdString(NumberFormat::money(bill.totalDiscount)))
            .arg(QString::fromStdString(NumberFormat::money(bill.totalPayable))));
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

void PosWindow::generateBill()
{
    if (cart_.empty())
    {
        QMessageBox::warning(this, "Generate Bill", "The cart is empty.");
        return;
    }

    if (customerInput_->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Generate Bill", "Enter a customer name.");
        return;
    }

    Order order;
    order.date = OrderManager::getCurrentDate();
    order.time = OrderManager::getCurrentTime();
    order.orderId = OrderManager::generateOrderId(order.date, orderPath_);

    if (order.orderId.empty())
    {
        QMessageBox::critical(this, "Generate Bill",
                              "Could not allocate an order id.");
        return;
    }

    order.customerName = customerInput_->text().trimmed().toStdString();
    order.contact = contactInput_->text().trimmed().toStdString();
    order.paymentMode = paymentModeInput_->currentText().toStdString();
    order.merchant = merchantInput_->text().trimmed().toStdString();
    order.status = "paid";

    std::vector<OrderDetail> details;
    details.reserve(cart_.size());
    for (const CartLine &line : cart_)
    {
        OrderDetail detail;
        detail.orderId = order.orderId;
        detail.productId = line.product.id();
        detail.productBarcode = line.product.barcode();
        detail.quantity = line.quantity;
        details.push_back(detail);
    }

    try
    {
        const Bill quote = billing_.quote(order, details);
        order.paymentAmount = quote.totalPayable;

        if (!OrderManager::saveOrderTransaction(order, details,
                                                orderPath_, detailPath_))
        {
            QMessageBox::critical(this, "Generate Bill",
                                  "Could not save Order.txt / OrderDetail.txt.");
            return;
        }

        const Bill bill = billing_.generateBill(order, details);
        showBill(order, bill, cart_);

        cart_.clear();
        lastProduct_.reset();
        customerInput_->clear();
        contactInput_->clear();
        merchantInput_->clear();
        updateCart();
        refreshRecentOrders();
        emit orderSaved();

        setStatus(QString("Bill %1 generated for order %2.")
                      .arg(QString::fromStdString(bill.billId))
                      .arg(QString::fromStdString(order.orderId)));
    }
    catch (const std::exception &error)
    {
        QMessageBox::critical(this, "Generate Bill",
                              QString::fromStdString(error.what()));
    }
}

void PosWindow::showBill(const Order &order, const Bill &bill,
                         const std::vector<CartLine> &lines)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Bill " + QString::fromStdString(bill.billId));
    dialog.resize(540, 620);

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

    QPushButton *closeButton = new QPushButton("Close", &dialog);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(closeButton);

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
