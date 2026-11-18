#ifndef BARCODE_SCANNER_SERVICE_H
#define BARCODE_SCANNER_SERVICE_H

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <string>

// Qt adapter around the Qt-free BarcodeScanPipeline.
//
// The UI only connects to barcodeDetected(); it never duplicates decode logic.
// Live frames are throttled, and the service stops scanning after the first
// successful decode (call start() again for the next item).
class BarcodeScannerService : public QObject
{
    Q_OBJECT

public:
    explicit BarcodeScannerService(QObject *parent = nullptr);

    // Live scanning. start() resets the one-shot latch; stop() pauses it.
    void start();
    void stop();
    bool isScanning() const;

    // One-shot decode for uploaded images and the stop-camera fallback.
    // Returns an empty string when the image has no readable barcode.
    std::string decode(const QImage &image) const;

public slots:
    // Feed a camera frame while scanning. Frames are throttled and the
    // service stops after the first successful decode.
    void processFrame(const QImage &image);

signals:
    // Emitted once per start() with the decoded 6-digit barcode.
    void barcodeDetected(const std::string &barcode);

private:
    QElapsedTimer throttle_;
    bool scanning_ = false;
    bool emitted_ = false;
};

#endif // BARCODE_SCANNER_SERVICE_H
