#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "decoder/barcodeDecoder.h"
#include "decoder/barcodeImageScanner.h"
#include "encoder/barcodeEncoder.h"
#include "renderer/BarcodeWidget.h" 

class QLineEdit;
class QLabel;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    QLineEdit *barcodeInput;

    BarcodeWidget *barcodeRendererWidget; 
    
    QLabel *readerPreview;
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
};

#endif