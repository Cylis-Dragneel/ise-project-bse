#include "BarcodeWidget.h"
#include <QPainter>
#include <QBrush>
#include <QPen>

BarcodeWidget::BarcodeWidget(QWidget *parent): QWidget(parent), narrowWidth(2), wideWidth(6)
{
    // Default empty or placeholder pattern
    binaryPattern = "";
}

void BarcodeWidget::setPattern(const std::string &pattern)
{
    binaryPattern = pattern;
    update();
}

void BarcodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    //Draw a clean white background for the barcode area
    painter.fillRect(rect(), Qt::white);

    if (binaryPattern.empty())
    {
        return; // Nothing to draw
    }

    int xPos = 20;                 // Left margin (quiet zone padding)
    int yPos = 20;                 // Top margin
    int barHeight = height() - 40; // Bar height with margins

    if (barHeight <= 0)
        barHeight = 80; //Fallback height if widget is too small

    // Iterate through the binary string
    for (size_t i = 0; i < binaryPattern.length(); ++i)
    {
        char element = binaryPattern[i];

        //Determine element width (narrow vs wide)
        int currentWidth = (element == '1') ? wideWidth : narrowWidth;

        // Even index = Bar (Black), Odd index = Space (White/Skip)
        bool isBar = (i % 2 == 0);

        if (isBar)
        {
            // Draw the black vertical bar
            painter.fillRect(xPos, yPos, currentWidth, barHeight, Qt::black);
        }
        //if the chars is a space then we simply advance the xPos to leave space
        xPos += currentWidth;
    }
}
