#ifndef PRODUCT_DESCRIPTION_DIALOG_H
#define PRODUCT_DESCRIPTION_DIALOG_H

#include <QDialog>

class Product;

// Shows the full product specifications (the "Description" view). The text
// comes from the Qt-free ProductFormatter, so the dialog and any future report
// show exactly the same fields.
class ProductDescriptionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProductDescriptionDialog(const Product& product,
                                      QWidget *parent = nullptr);
};

#endif // PRODUCT_DESCRIPTION_DIALOG_H
