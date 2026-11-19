#include "ProductDescriptionDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QString>

ProductDescriptionDialog::ProductDescriptionDialog(const Product& p, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Product Details");
    setMinimumWidth(300);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QString name = QString::fromStdString(p.name).replace("_", " ");
    
    layout->addWidget(new QLabel("<b>Name:</b> " + name));
    layout->addWidget(new QLabel("<b>Acronym:</b> " + QString::fromStdString(p.acronym)));
    layout->addWidget(new QLabel("<b>Barcode:</b> " + QString::fromStdString(p.barcode)));
    layout->addWidget(new QLabel("<b>Unit:</b> " + QString::fromStdString(p.unit)));
    layout->addWidget(new QLabel("<b>Price:</b> PKR " + QString::number(p.unitPrice, 'f', 2)));
    layout->addWidget(new QLabel("<b>Tax:</b> PKR " + QString::number(p.tax, 'f', 2)));
    layout->addWidget(new QLabel("<b>Discount:</b> PKR " + QString::number(p.discount, 'f', 2)));
    layout->addWidget(new QLabel("<b>Available Qty:</b> " + QString::number(p.availableQty)));
}