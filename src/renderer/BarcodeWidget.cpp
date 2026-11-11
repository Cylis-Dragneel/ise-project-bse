#include "BarcodeWidget.h"
#include <QPainter>
#include <QBrush>
#include <QPen>

BarcodeWidget::BarcodeWidget(QWidget *parent): QWidget(parent), narrowWidth(2), wideWidth(6)
{
    //Default empty or placeholder pattern
    binaryPattern = "";
}

void BarcodeWidget::setPattern(const std::string &pattern)
{
    binaryPattern = pattern;
    update();
}

bool BarcodeWidget::hasPattern() const
{
    return !binaryPattern.empty();
}

QImage BarcodeWidget::renderImage(int scale, int margin, int barHeight) const
{
    if (binaryPattern.empty() || scale < 1 || margin < 0 || barHeight <= 0) {
        return QImage();
    }

    const int narrow = narrowWidth * scale;
    const int wide = wideWidth * scale;

    int barcodeWidth = 0;
    for (char element : binaryPattern) {
        barcodeWidth += (element == '1') ? wide : narrow;
    }

    QImage image(barcodeWidth + 2 * margin,
                 barHeight + 2 * margin,
                 QImage::Format_RGB32);
    image.fill(Qt::white);

    QPainter painter(&image);

    int xPos = margin;
    for (size_t i = 0; i < binaryPattern.length(); ++i) {
        const int currentWidth = (binaryPattern[i] == '1') ? wide : narrow;
        const bool isBar = (i % 2 == 0);

        if (isBar) {
            painter.fillRect(xPos, margin, currentWidth, barHeight, Qt::black);
        }

        xPos += currentWidth;
    }

    painter.end();
    return image;
}

void BarcodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    //white background for the barcode area
    painter.fillRect(rect(), Qt::white);

    if (binaryPattern.empty()) {
        return; // Nothing to draw
    }

    //Calculate total width of the barcode pattern
    int totalBarcodeWidth = 0;
    for (char element : binaryPattern) {
        totalBarcodeWidth += (element == '1') ? wideWidth : narrowWidth;
    }

    //Compute starting X position to center it horizontally
    int xPos = (width() - totalBarcodeWidth) / 2;
    if (xPos < 10) {
        xPos = 10; //Minimum left margin safety fallback
    }

    //Compute bar height and vertical centering
    int barHeight = height() - 40;
    if (barHeight <= 0) {
        barHeight = 80;
    }
    int yPos = (height() - barHeight) / 2;

    //Iterate and draw each bar
    for (size_t i = 0; i < binaryPattern.length(); ++i) {
        char element = binaryPattern[i];

        int currentWidth = (element == '1') ? wideWidth : narrowWidth;
        bool isBar = (i % 2 == 0); // Even index = Bar, Odd index = Space

        if (isBar) {
            painter.fillRect(xPos, yPos, currentWidth, barHeight, Qt::black);
        }

        xPos += currentWidth;
    }
}
