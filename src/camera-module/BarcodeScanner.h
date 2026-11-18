#ifndef BARCODESCANNER_H
#define BARCODESCANNER_H

#include <QImage>
#include <string>

class BarcodeScanner {
public:
    static std::string extractBarcodeString(const QImage &image);
};

#endif // BARCODESCANNER_H
