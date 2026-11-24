#ifndef BILL_PDF_WRITER_H
#define BILL_PDF_WRITER_H

#include <QString>
#include "Bill.h"

namespace inc3 {

// Qt-side PDF receipt renderer (uses QPdfWriter + QPainter).
// Lives in src/renderer/ so qrcore remains Qt-free.
class BillPdfWriter
{
public:
    // Suggested filename convention: Bill_<billId>.pdf
    static QString defaultFileName(const BillHeader& bill);

    // Renders a formatted receipt PDF to filePath. Returns true on success.
    static bool writeBillPdf(const BillHeader& bill,
                             const QString& filePath,
                             QString* errorOut = nullptr);
};

} // namespace inc3

#endif // BILL_PDF_WRITER_H
