#ifndef BARCODE_WIDGET_H
#define BARCODE_WIDGET_H

#include <QWidget>
#include <string>

class BarcodeWidget : public QWidget {
    Q_OBJECT
public:
    explicit BarcodeWidget(QWidget *parent = nullptr);

    // Sets the binary pattern string received from BarcodeEncoder and triggers a redraw
    void setPattern(const std::string& pattern);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::string binaryPattern;
    int narrowWidth; // Width of a narrow bar/space in pixels
    int wideWidth;   // Width of a wide bar/space in pixels
};

#endif
