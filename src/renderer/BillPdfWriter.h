#ifndef BILL_PDF_WRITER_H
#define BILL_PDF_WRITER_H

#include <QString>
#include "Bill.h"

class BillPdfWriter
{
public:
    static QString defaultFileName(const Bill& bill);
    static bool writeBillPdf(const Bill& bill,
                             const QString& filePath,
                             QString* errorOut = nullptr);
};

#endif // BILL_PDF_WRITER_H