#include "mainwindow.h"

#include "camera/CameraHandler.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QStatusBar>
#include <QRegularExpressionValidator>
#include <QVideoWidget>
#include <exception>
#include <string>
#include <vector>

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

    // Integrated custom BarcodeWidget renderer
    barcodeRendererWidget = new BarcodeWidget(generatorPanel);
    barcodeRendererWidget->setMinimumHeight(200);
    barcodeRendererWidget->setStyleSheet("border: 1px solid gray; background: white;");

    saveButton = new QPushButton("Save Barcode Image");
    saveButton->setEnabled(false);

    clearButton = new QPushButton("Clear");

    generatorLayout->addWidget(inputLabel);
    generatorLayout->addWidget(barcodeInput);
    generatorLayout->addWidget(generateButton);
    generatorLayout->addWidget(barcodeRendererWidget);
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

    cameraPreview = new QVideoWidget();
    cameraPreview->setMinimumHeight(200);
    cameraPreview->hide();

    startCameraButton = new QPushButton("Start Camera");
    stopCameraButton = new QPushButton("Stop Camera");
    stopCameraButton->setEnabled(false);

    decodedNumber = new QLabel("Decoded Number: Not available");
    decodedNumber->setAlignment(Qt::AlignCenter);

    readerLayout->addWidget(uploadButton);
    readerLayout->addWidget(readerPreview);
    readerLayout->addWidget(cameraPreview);
    readerLayout->addWidget(startCameraButton);
    readerLayout->addWidget(stopCameraButton);
    readerLayout->addWidget(decodedNumber);

    panelsLayout->addWidget(generatorPanel);
    panelsLayout->addWidget(readerPanel);

    mainLayout->addLayout(panelsLayout);

    cameraHandler = new CameraHandler(cameraPreview, this);

    connect(cameraHandler, &CameraHandler::errorOccurred,
            this, [this](const QString &message) {
                cameraHandler->stopCamera();
                cameraPreview->hide();
                readerPreview->show();
                startCameraButton->setEnabled(true);
                stopCameraButton->setEnabled(false);
                statusBar()->showMessage(message);
            });

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

    connect(saveButton, &QPushButton::clicked,
            this, &MainWindow::saveBarcode);

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

MainWindow::~MainWindow()
{
    if (cameraHandler)
    {
        cameraHandler->stopCamera();
    }
}

// Generate button
void MainWindow::generateBarcode()
{
    QString textInput = barcodeInput->text();

    bool ok;
    int numberValue = textInput.toInt(&ok);

    if (!ok)
    {
        statusBar()->showMessage("Please enter a valid numeric value.");
        return;
    }

    try
    {
        // Call the core C++ encoder
        std::string binaryPattern = encoder.encode(numberValue);

        // Pass the binary pattern to your custom barcode renderer widget
        barcodeRendererWidget->setPattern(binaryPattern);

        saveButton->setEnabled(true); // Enable saving once generated
        statusBar()->showMessage("Barcode generated successfully!");
    }
    catch (const std::out_of_range& e)
    {
        statusBar()->showMessage(e.what());
    }
    catch (const std::exception& e)
    {
        statusBar()->showMessage(e.what());
    }
}

// Save button
void MainWindow::saveBarcode()
{
    if (!barcodeRendererWidget->hasPattern())
    {
        statusBar()->showMessage("Generate a barcode before saving.");
        return;
    }

    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Barcode Image",
        "barcode.png",
        "PNG image (*.png);;JPEG image (*.jpg *.jpeg);;Bitmap image (*.bmp)",
        &selectedFilter);

    if (fileName.isEmpty())
        return;

    if (QFileInfo(fileName).suffix().isEmpty())
    {
        if (selectedFilter.contains("JPEG"))
            fileName += ".jpg";
        else if (selectedFilter.contains("Bitmap"))
            fileName += ".bmp";
        else
            fileName += ".png";
    }

    const QImage image = barcodeRendererWidget->renderImage();

    if (image.isNull())
    {
        statusBar()->showMessage("There is no barcode to save.");
        return;
    }

    if (!image.save(fileName))
    {
        statusBar()->showMessage("Unable to save the barcode image.");
        return;
    }

    statusBar()->showMessage(QString("Barcode saved to %1").arg(fileName));
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

    QImage image(fileName);

    if (image.isNull())
    {
        statusBar()->showMessage("Unable to load image.");
        return;
    }

    if (cameraHandler->isRunning())
    {
        stopCamera();
    }

    readerPreview->setPixmap(
        QPixmap::fromImage(image).scaled(readerPreview->size(),
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation));

    // Composite onto white so transparent images do not turn into black
    // pixels when converted to grayscale.
    QImage opaque(image.size(), QImage::Format_RGB32);
    opaque.fill(Qt::white);
    QPainter painter(&opaque);
    painter.drawImage(0, 0, image);
    painter.end();

    const QImage grayscale = opaque.convertToFormat(QImage::Format_Grayscale8);

    const std::vector<std::string> candidates = imageScanner.scan(
        grayscale.constBits(),
        grayscale.width(),
        grayscale.height(),
        grayscale.bytesPerLine());

    for (const std::string &pattern : candidates)
    {
        try
        {
            const int number = decoder.decode(pattern);

            decodedNumber->setText(QString("Decoded Number: %1").arg(number));
            statusBar()->showMessage("Barcode decoded successfully.");
            return;
        }
        catch (const std::exception &)
        {
            // This scan line did not contain a decodable barcode. Try the next.
        }
    }

    decodedNumber->setText("Decoded Number: Not available");
    statusBar()->showMessage("No readable barcode was found in this image.");
}

// Clear generator
void MainWindow::clearGenerator()
{
    barcodeInput->clear();
    barcodeRendererWidget->setPattern(""); // Clear the rendered barcode
    saveButton->setEnabled(false);

    statusBar()->showMessage("Generator cleared.");
}

// Start camera
void MainWindow::startCamera()
{
    if (cameraHandler->isRunning())
    {
        return;
    }

    if (!cameraHandler->startCamera())
    {
        // errorOccurred already reset the UI and reported the problem.
        return;
    }

    readerPreview->hide();
    cameraPreview->show();
    startCameraButton->setEnabled(false);
    stopCameraButton->setEnabled(true);
    statusBar()->showMessage("Camera started.");
}

// Stop camera
void MainWindow::stopCamera()
{
    cameraHandler->stopCamera();

    cameraPreview->hide();
    readerPreview->show();
    startCameraButton->setEnabled(true);
    stopCameraButton->setEnabled(false);
    statusBar()->showMessage("Camera stopped.");
}