
#include <QApplication>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QStatusBar> 
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>
#include <QWidget>

class MainWindow : public QMainWindow {
public:
  MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
    setWindowTitle("Barcode Generator and Reader");
    resize(1000, 650);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(25, 20, 25, 20);
    mainLayout->setSpacing(20);

    // Application heading
    QLabel *heading = new QLabel("Barcode Generator & Reader");
    heading->setAlignment(Qt::AlignCenter);
    heading->setStyleSheet(
        "font-size: 25px; font-weight: bold; color: #26354A;");

    mainLayout->addWidget(heading);

    // Two main panels
    QHBoxLayout *panelsLayout = new QHBoxLayout;
    panelsLayout->setSpacing(20);

    // ================= GENERATOR PANEL =================

    QFrame *generatorFrame = new QFrame;
    generatorFrame->setStyleSheet(
        "QFrame { background-color: #F5F7FA;"
        "border: 1px solid #D5DCE5; border-radius: 10px; }");

    QVBoxLayout *generatorLayout = new QVBoxLayout(generatorFrame);
    generatorLayout->setContentsMargins(20, 20, 20, 20);
    generatorLayout->setSpacing(15);

    QLabel *generatorTitle = new QLabel("Generate Barcode");
    generatorTitle->setStyleSheet(
        "font-size: 19px; font-weight: bold; border: none;");

    QLabel *numberLabel = new QLabel("Enter 6-digit number:");

    QLineEdit *numberInput = new QLineEdit;
    numberInput->setObjectName("numberInput");
    numberInput->setPlaceholderText("Example: 123456");
    numberInput->setMaxLength(6);
    numberInput->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[1-9][0-9]{0,5}"), numberInput));
    numberInput->setStyleSheet("padding: 10px; border: 1px solid #B8C3D1;"
                               "border-radius: 5px; background: white;");

    QPushButton *generateButton = new QPushButton("Generate Barcode");
    generateButton->setObjectName("generateButton");

    QLabel *barcodePreview = new QLabel("Generated barcode will appear here");
    barcodePreview->setObjectName("barcodePreview");
    barcodePreview->setAlignment(Qt::AlignCenter);
    barcodePreview->setMinimumHeight(180);
    barcodePreview->setStyleSheet(
        "background: white; border: 1px dashed #9AA7B5;"
        "border-radius: 6px; color: #68778A;");

    QPushButton *saveButton = new QPushButton("Save Barcode Image");
    saveButton->setObjectName("saveButton");
    saveButton->setEnabled(false);

    QPushButton *clearButton = new QPushButton("Clear");
    clearButton->setObjectName("clearButton");

    generatorLayout->addWidget(generatorTitle);
    generatorLayout->addWidget(numberLabel);
    generatorLayout->addWidget(numberInput);
    generatorLayout->addWidget(generateButton);
    generatorLayout->addWidget(barcodePreview, 1);
    generatorLayout->addWidget(saveButton);
    generatorLayout->addWidget(clearButton);

    // ================= READER PANEL =================

    QFrame *readerFrame = new QFrame;
    readerFrame->setStyleSheet(
        "QFrame { background-color: #F5F7FA;"
        "border: 1px solid #D5DCE5; border-radius: 10px; }");

    QVBoxLayout *readerLayout = new QVBoxLayout(readerFrame);
    readerLayout->setContentsMargins(20, 20, 20, 20);
    readerLayout->setSpacing(15);

    QLabel *readerTitle = new QLabel("Read Barcode");
    readerTitle->setStyleSheet(
        "font-size: 19px; font-weight: bold; border: none;");

    QPushButton *uploadButton = new QPushButton("Upload Barcode Image");
    uploadButton->setObjectName("uploadButton");

    QLabel *cameraPreview = new QLabel("Camera preview will appear here");
    cameraPreview->setObjectName("cameraPreview");
    cameraPreview->setAlignment(Qt::AlignCenter);
    cameraPreview->setMinimumHeight(180);
    cameraPreview->setStyleSheet("background: #263238; color: white;"
                                 "border-radius: 6px; border: none;");

    QHBoxLayout *cameraButtons = new QHBoxLayout;

    QPushButton *startCameraButton = new QPushButton("Start Camera");
    startCameraButton->setObjectName("startCameraButton");

    QPushButton *stopCameraButton = new QPushButton("Stop Camera");
    stopCameraButton->setObjectName("stopCameraButton");

    cameraButtons->addWidget(startCameraButton);
    cameraButtons->addWidget(stopCameraButton);

    QLabel *resultLabel = new QLabel("Decoded Number:");
    resultLabel->setStyleSheet("font-weight: bold; border: none;");

    QLabel *resultValue = new QLabel("Not scanned yet");
    resultValue->setObjectName("resultValue");
    resultValue->setAlignment(Qt::AlignCenter);
    resultValue->setMinimumHeight(45);
    resultValue->setStyleSheet("background: white; border: 1px solid #B8C3D1;"
                               "border-radius: 5px; font-size: 18px;");

    readerLayout->addWidget(readerTitle);
    readerLayout->addWidget(uploadButton);
    readerLayout->addWidget(cameraPreview, 1);
    readerLayout->addLayout(cameraButtons);
    readerLayout->addWidget(resultLabel);
    readerLayout->addWidget(resultValue);

    panelsLayout->addWidget(generatorFrame);
    panelsLayout->addWidget(readerFrame);

    mainLayout->addLayout(panelsLayout, 1);

    // Status bar
    statusBar()->showMessage("Ready");

    // ================= BUTTON CONNECTIONS =================

    connect(clearButton, &QPushButton::clicked, this, [=]() {
      numberInput->clear();
      barcodePreview->setText("Generated barcode will appear here");
      barcodePreview->setPixmap(QPixmap());
      saveButton->setEnabled(false);
      statusBar()->showMessage("Input cleared");
    });

    connect(generateButton, &QPushButton::clicked, this, [=]() {
      QString number = numberInput->text();

      if (!QRegularExpression("^[1-9][0-9]{5}$").match(number).hasMatch()) {
        QMessageBox::warning(this, "Invalid Input",
                             "Please enter exactly 6 digits.\n"
                             "The first digit cannot be zero.");
        return;
      }

      statusBar()->showMessage(
          "Input accepted. Generator integration pending.");
    });

    connect(uploadButton, &QPushButton::clicked, this, [=]() {
      QString fileName =
          QFileDialog::getOpenFileName(this, "Select Barcode Image", "",
                                       "Images (*.png *.jpg *.jpeg *.bmp)");

      if (!fileName.isEmpty()) {
        QPixmap image(fileName);

        if (image.isNull()) {
          QMessageBox::warning(this, "Error", "Unable to open this image.");
          return;
        }

        cameraPreview->setPixmap(image.scaled(cameraPreview->size(),
                                              Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));

        statusBar()->showMessage(
            "Image uploaded. Decoder integration pending.");
      }
    });

    connect(startCameraButton, &QPushButton::clicked, this,
            [=]() { statusBar()->showMessage("Camera integration pending."); });

    connect(stopCameraButton, &QPushButton::clicked, this, [=]() {
      statusBar()->showMessage("Camera stopped.");
      cameraPreview->setText("Camera preview will appear here");
      cameraPreview->setPixmap(QPixmap());
    });

    // General button appearance
    setStyleSheet("QPushButton {"
                  "background-color: #2864A5;"
                  "color: white;"
                  "padding: 10px;"
                  "border: none;"
                  "border-radius: 5px;"
                  "font-weight: bold;"
                  "}"
                  "QPushButton:hover { background-color: #194D85; }"
                  "QPushButton:disabled { background-color: #AAB4C0; }");
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  MainWindow window;
  window.show();

  return app.exec();
}
