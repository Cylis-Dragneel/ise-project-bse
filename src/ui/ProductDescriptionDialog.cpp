#include "ProductDescriptionDialog.h"

#include "catalog/Product.h"
#include "catalog/ProductFormatter.h"

#include <QFontDatabase>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

ProductDescriptionDialog::ProductDescriptionDialog(const Product& product,
                                                   QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Product Details");
    resize(440, 340);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QTextEdit *text = new QTextEdit(this);
    text->setReadOnly(true);
    text->setPlainText(
        QString::fromStdString(ProductFormatter::describe(product)));
    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(text);

    QPushButton *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeButton);
}
