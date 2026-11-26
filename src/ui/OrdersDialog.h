#ifndef ORDERS_DIALOG_H
#define ORDERS_DIALOG_H

#include <QDialog>
#include <string>

class QLineEdit;
class QTableWidget;

// Manager/cashier order browser (requirement 3): search by order id or
// customer and open an order for editing. Read-only; the caller loads the
// detail rows and puts them in the cart.
class OrdersDialog : public QDialog
{
    Q_OBJECT
public:
    OrdersDialog(const std::string &orderPath,
                 QWidget *parent = nullptr);

    // Empty when nothing was opened.
    std::string selectedOrderId() const { return selectedOrderId_; }

private:
    std::string orderPath_;
    std::string selectedOrderId_;

    QLineEdit *search_ = nullptr;
    QTableWidget *table_ = nullptr;

    void refresh();
    void openSelected();
};

#endif // ORDERS_DIALOG_H
