#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "decoder/barcodeDecoder.h"
#include "decoder/barcodeImageScanner.h"
#include "encoder/barcodeEncoder.h"
#include "renderer/BarcodeWidget.h" 

class QImage;
class QLineEdit;
class QLabel;
class QPushButton;
class QVideoWidget;
class CameraHandler;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    QLineEdit *barcodeInput;

    BarcodeWidget *barcodeRendererWidget; 
    
    QLabel *readerPreview;
    QVideoWidget *cameraPreview;
    CameraHandler *cameraHandler;
    QLabel *decodedNumber;

    QPushButton *generateButton;
    QPushButton *saveButton;
    QPushButton *clearButton;
    QPushButton *uploadButton;
    QPushButton *startCameraButton;
    QPushButton *stopCameraButton;

    BarcodeEncoder encoder;
    BarcodeDecoder decoder;
    BarcodeImageScanner imageScanner;

    void generateBarcode();
    void saveBarcode();
    void uploadImage();
    void clearGenerator();
    void startCamera();
    void stopCamera();

    // Stops the preview and, when asked, tries to decode the last frame.
    void stopCameraInternal(bool decodeFrame);
    // Scans the image and updates decodedNumber. Returns true on success.
    bool decodeImage(const QImage &image);
};

#endif