
#include "mainwindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QPixmap>
#include <QStatusBar>
#include <QRegularExpressionValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Barcode Generator and Reader");
    resize(1000, 650);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    QLabel *heading = new QLabel("Barcode Generator & Reader");
    heading->setAlignment(Qt::AlignCenter);
    heading->setStyleSheet("font-size: 26px; font-weight: bold;");

    mainLayout->addWidget(heading);

    QHBoxLayout *panelsLayout = new QHBoxLayout();

    // Generator panel
    QGroupBox *generatorPanel = new QGroupBox("Barcode Generator");
    QVBoxLayout *generatorLayout = new QVBoxLayout(generatorPanel);

    QLabel *inputLabel = new QLabel("Enter 6-digit number:");

    barcodeInput = new QLineEdit();
    barcodeInput->setMaxLength(6);
    barcodeInput->setPlaceholderText("Example: 234567");

    QRegularExpression regex("[1-9][0-9]{0,5}");
    barcodeInput->setValidator(
        new QRegularExpressionValidator(regex, this));

    generateButton = new QPushButton("Generate Barcode");
    generatorPreview = new QLabel("Barcode Preview");
    generatorPreview->setAlignment(Qt::AlignCenter);
    generatorPreview->setMinimumHeight(200);
    generatorPreview->setStyleSheet(
        "border: 1px solid gray; background: #f5f5f5;");

    saveButton = new QPushButton("Save Barcode Image");
    saveButton->setEnabled(false);

    clearButton = new QPushButton("Clear");

    generatorLayout->addWidget(inputLabel);
    generatorLayout->addWidget(barcodeInput);
    generatorLayout->addWidget(generateButton);
    generatorLayout->addWidget(generatorPreview);
    generatorLayout->addWidget(saveButton);
    generatorLayout->addWidget(clearButton);

    // Reader panel
    QGroupBox *readerPanel = new QGroupBox("Barcode Reader");
    QVBoxLayout *readerLayout = new QVBoxLayout(readerPanel);

    uploadButton = new QPushButton("Upload Barcode Image");

    readerPreview = new QLabel("Uploaded Image Preview");
    readerPreview->setAlignment(Qt::AlignCenter);
    readerPreview->setMinimumHeight(200);
    readerPreview->setStyleSheet(
        "border: 1px solid gray; background: #f5f5f5;");

    startCameraButton = new QPushButton("Start Camera");
    stopCameraButton = new QPushButton("Stop Camera");

    decodedNumber = new QLabel("Decoded Number: Not available");
    decodedNumber->setAlignment(Qt::AlignCenter);

    readerLayout->addWidget(uploadButton);
    readerLayout->addWidget(readerPreview);
    readerLayout->addWidget(startCameraButton);
    readerLayout->addWidget(stopCameraButton);
    readerLayout->addWidget(decodedNumber);

    panelsLayout->addWidget(generatorPanel);
    panelsLayout->addWidget(readerPanel);

    mainLayout->addLayout(panelsLayout);

    // Button styling
    setStyleSheet(
        "QPushButton {"
        "padding: 9px;"
        "background-color: #2878c8;"
        "color: white;"
        "border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "background-color: #155a9c;"
        "}"
        "QGroupBox {"
        "font-weight: bold;"
        "}"
    );

    // Button connections
    connect(generateButton, &QPushButton::clicked,
            this, &MainWindow::generateBarcode);

    connect(uploadButton, &QPushButton::clicked,
            this, &MainWindow::uploadImage);

    connect(clearButton, &QPushButton::clicked,
            this, &MainWindow::clearGenerator);

    connect(startCameraButton, &QPushButton::clicked,
            this, &MainWindow::startCamera);

    connect(stopCameraButton, &QPushButton::clicked,
            this, &MainWindow::stopCamera);

    statusBar()->showMessage("Application ready");
}

// Generate button
void MainWindow::generateBarcode()
{
    QString number = barcodeInput->text();

    if (number.length() != 6 || number[0] == '0')
    {
        statusBar()->showMessage(
            "Please enter exactly 6 digits. First digit cannot be zero.");
        return;
    }

    generatorPreview->setText(
        "Barcode generation will be connected here.");

    statusBar()->showMessage(
        "Input accepted. Generator integration pending.");
}

// Upload image button
void MainWindow::uploadImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Barcode Image",
        "",
        "Images (*.png *.jpg *.jpeg *.bmp)");

    if (fileName.isEmpty())
        return;

    QPixmap image(fileName);

    if (image.isNull())
    {
        statusBar()->showMessage("Unable to load image.");
        return;
    }

    readerPreview->setPixmap(
        image.scaled(readerPreview->size(),
                     Qt::KeepAspectRatio,
                     Qt::SmoothTransformation));

    statusBar()->showMessage(
        "Image uploaded. Decoder integration pending.");
}

// Clear generator
void MainWindow::clearGenerator()
{
    barcodeInput->clear();
    generatorPreview->setText("Barcode Preview");
    saveButton->setEnabled(false);

    statusBar()->showMessage("Generator cleared.");
}

// Start camera placeholder
void MainWindow::startCamera()
{
    statusBar()->showMessage(
        "Camera integration pending.");
}

// Stop camera placeholder
void MainWindow::stopCamera()
{
    statusBar()->showMessage(
        "Camera stopped.");
    readerPreview->setText("Uploaded Image Preview");
    readerPreview->setPixmap(QPixmap());
}
