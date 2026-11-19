#ifndef PRODUCTDESCRIPTIONDIALOG_H
#define PRODUCTDESCRIPTIONDIALOG_H

#include <QDialog>
#include "../models/Product.h"

class QLabel;

class ProductDescriptionDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProductDescriptionDialog(const Product& p, QWidget *parent = nullptr);
};

#endif