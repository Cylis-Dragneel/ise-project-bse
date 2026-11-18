#include "BarcodeScannerService.h"

#include "scanning/BarcodeScanPipeline.h"

#include <QPainter>

namespace {

constexpr qint64 kScanIntervalMs = 250;

QImage toGrayscale(const QImage &image)
{
    if (image.isNull())
    {
        return {};
    }

    // Composite onto white so transparent images do not turn into black
    // pixels when converted to grayscale.
    QImage opaque(image.size(), QImage::Format_RGB32);
    opaque.fill(Qt::white);
    QPainter painter(&opaque);
    painter.drawImage(0, 0, image);
    painter.end();

    return opaque.convertToFormat(QImage::Format_Grayscale8);
}

} // namespace

BarcodeScannerService::BarcodeScannerService(QObject *parent)
    : QObject(parent)
{
}

void BarcodeScannerService::start()
{
    scanning_ = true;
    emitted_ = false;
    throttle_.invalidate();
}

void BarcodeScannerService::stop()
{
    scanning_ = false;
}

bool BarcodeScannerService::isScanning() const
{
    return scanning_;
}

std::string BarcodeScannerService::decode(const QImage &image) const
{
    const QImage grayscale = toGrayscale(image);
    if (grayscale.isNull())
    {
        return {};
    }

    return BarcodeScanPipeline::decode(
        grayscale.constBits(),
        grayscale.width(),
        grayscale.height(),
        static_cast<int>(grayscale.bytesPerLine()));
}

void BarcodeScannerService::processFrame(const QImage &image)
{
    if (!scanning_ || emitted_)
    {
        return;
    }

    // Throttle scanning independently of the camera frame rate.
    if (throttle_.isValid() && throttle_.elapsed() < kScanIntervalMs)
    {
        return;
    }
    throttle_.start();

    const std::string barcode = decode(image);
    if (barcode.empty())
    {
        return;
    }

    // Stop after the first success; the caller can start() again for the
    // next item.
    emitted_ = true;
    scanning_ = false;
    emit barcodeDetected(barcode);
}
