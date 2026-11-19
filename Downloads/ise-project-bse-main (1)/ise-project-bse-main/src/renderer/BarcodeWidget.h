#ifndef BARCODE_WIDGET_H
#define BARCODE_WIDGET_H

#include <QImage>
#include <QWidget>
#include <string>

class BarcodeWidget : public QWidget {
    Q_OBJECT
public:
    explicit BarcodeWidget(QWidget *parent = nullptr);

    // Sets the binary pattern string received from BarcodeEncoder and triggers a redraw
    void setPattern(const std::string& pattern);

    // True once setPattern() has been called with a non-empty pattern.
    bool hasPattern() const;

    // Renders the barcode to a standalone image suitable for saving.
    // scale multiplies the on-screen narrow/wide widths, margin is the
    // white quiet zone around the bars, and barHeight is the bar height.
    QImage renderImage(int scale = 2, int margin = 24, int barHeight = 160) const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::string binaryPattern;
    int narrowWidth; // Width of a narrow bar/space in pixels
    int wideWidth;   // Width of a wide bar/space in pixels
};

#endif
