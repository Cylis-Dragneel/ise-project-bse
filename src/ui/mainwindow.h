#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

    void generateBarcode();
    void uploadImage();
    void clearGenerator();
    void startCamera();
    void stopCamera();
};

#endif