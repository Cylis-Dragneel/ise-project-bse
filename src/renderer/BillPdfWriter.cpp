#include "BillPdfWriter.h"

#include "billing/BillCalculator.h"

#include <QFont>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QRect>

QString BillPdfWriter::defaultFileName(const Bill& bill)
{
    return QString("Bill_%1.pdf").arg(QString::fromStdString(bill.billId));
}

bool BillPdfWriter::writeBillPdf(const Bill& bill,
                               const QString& filePath,
                               QString* errorOut)
{
    if (filePath.trimmed().isEmpty())
    {
        if (errorOut) *errorOut = "PDF output file path cannot be empty.";
        return false;
    }

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive())
    {
        if (errorOut) *errorOut = "Failed to open PDF writer for: " + filePath;
        return false;
    }

    const int pageWidth = writer.width();
    const int pageHeight = writer.height();
    int y = 100;

    const QFont titleFont("SansSerif", 16, QFont::Bold);
    const QFont sectionFont("SansSerif", 11, QFont::Bold);
    const QFont regularFont("SansSerif", 10);
    const QFont monoFont("Monospace", 10);

    painter.setFont(titleFont);
    painter.drawText(QRect(0, y, pageWidth, 120), Qt::AlignCenter, "PAZARLA POS - OFFICIAL RECEIPT");
    y += 150;

    painter.setFont(regularFont);
    painter.drawLine(0, y, pageWidth, y);
    y += 50;

    auto drawMetaRow = [&](const QString& left, const QString& right) {
        painter.drawText(QRect(0, y, pageWidth / 2, 80), Qt::AlignLeft | Qt::AlignVCenter, left);
        painter.drawText(QRect(pageWidth / 2, y, pageWidth / 2, 80), Qt::AlignRight | Qt::AlignVCenter, right);
        y += 80;
    };

    drawMetaRow(QString("Bill ID: %1").arg(QString::fromStdString(bill.billId)),
                QString("Order ID: %1").arg(QString::fromStdString(bill.orderId)));
    drawMetaRow(QString("Customer: %1").arg(QString::fromStdString(bill.customerName)),
                QString("Status: %1").arg(QString::fromStdString(bill.paymentStatus)));
    drawMetaRow(QString("Date: %1").arg(QString::fromStdString(bill.billDate)),
                QString("Time: %1").arg(QString::fromStdString(bill.billTime)));

    y += 30;
    painter.drawLine(0, y, pageWidth, y);
    y += 50;

    painter.setFont(sectionFont);
    const int col1 = 0;
    const int col2 = static_cast<int>(pageWidth * 0.32);
    const int col3 = static_cast<int>(pageWidth * 0.54);
    const int col4 = static_cast<int>(pageWidth * 0.70);
    const int col5 = static_cast<int>(pageWidth * 0.85);

    painter.drawText(col1, y + 50, "Product");
    painter.drawText(col2, y + 50, "Unit Price");
    painter.drawText(col3, y + 50, "Qty");
    painter.drawText(col4, y + 50, "Discount");
    painter.drawText(col5, y + 50, "Amount");
    y += 80;
    painter.drawLine(0, y, pageWidth, y);
    y += 40;

    painter.setFont(monoFont);
    for (const BillLine& line : bill.lines)
    {
        if (y + 100 > pageHeight)
        {
            writer.newPage();
            y = 100;
            painter.setFont(monoFont);
        }
        const double lineNoTax =
            BillCalculator::roundMoney((line.unitPrice * line.quantity) - line.discountTotal);
        painter.drawText(col1, y + 50, QString::fromStdString(line.productAcronym));
        painter.drawText(col2, y + 50, QString::number(line.unitPrice, 'f', 2));
        painter.drawText(col3, y + 50, QString::number(line.quantity, 'f', 2));
        painter.drawText(col4, y + 50, QString::number(line.discountTotal, 'f', 2));
        painter.drawText(col5, y + 50, QString::number(lineNoTax, 'f', 2));
        y += 75;
    }

    if (y + 700 > pageHeight)
    {
        writer.newPage();
        y = 100;
    }

    y += 30;
    painter.drawLine(0, y, pageWidth, y);
    y += 60;

    auto drawTotalRow = [&](const QString& label, double amount, bool bold = false) {
        painter.setFont(bold ? sectionFont : regularFont);
        painter.drawText(QRect(pageWidth / 2, y, pageWidth / 4, 75), Qt::AlignLeft | Qt::AlignVCenter, label);
        painter.drawText(QRect(3 * pageWidth / 4, y, pageWidth / 4, 75), Qt::AlignRight | Qt::AlignVCenter,
                         QString("PKR %1").arg(amount, 0, 'f', 2));
        y += 75;
    };

    const double base =
        BillCalculator::roundMoney(bill.totalNoTax + bill.totalTax - bill.totalDiscount);
    const double afterSeasonal =
        BillCalculator::roundMoney(base - bill.seasonalDiscount);
    const double specialDiscountAmount =
        BillCalculator::roundMoney(afterSeasonal * bill.specialDiscountPercent / 100.0);

    drawTotalRow("Total (excl. Tax):", bill.totalNoTax);
    drawTotalRow("Total Tax:", bill.totalTax);
    drawTotalRow("Product Discounts:", -bill.totalDiscount);
    drawTotalRow("Seasonal Discount:", -bill.seasonalDiscount);
    drawTotalRow(QString("Special Discount (%1%):").arg(bill.specialDiscountPercent, 0, 'f', 2),
                 -specialDiscountAmount);

    y += 20;
    painter.drawLine(pageWidth / 2, y, pageWidth, y);
    y += 30;

    drawTotalRow("Total Payable:", bill.totalPayable, true);
    drawTotalRow("Amount Received:", bill.amountReceived);
    drawTotalRow("Balance Returned:", bill.balanceReturned, true);

    y += 80;
    painter.setFont(regularFont);
    painter.drawText(QRect(0, y, pageWidth, 80), Qt::AlignCenter, "Thank you for shopping with us!");

    painter.end();
    return true;
}