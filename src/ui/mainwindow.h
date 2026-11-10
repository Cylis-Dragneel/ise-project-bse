
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QLabel;
class QPushButton;

class MainWindow : public QMainWindow
{
public:
    MainWindow(QWidget *parent = nullptr);

private:
    QLineEdit *barcodeInput;

    QLabel *generatorPreview;
    QLabel *readerPreview;
    QLabel *decodedNumber;

    QPushButton *generateButton;
    QPushButton *saveButton;
    QPushButton *clearButton;
    QPushButton *uploadButton;
    QPushButton *startCameraButton;
    QPushButton *stopCameraButton;

    void generateBarcode();
    void uploadImage();
    void clearGenerator();
    void startCamera();
    void stopCamera();
};

#endif
