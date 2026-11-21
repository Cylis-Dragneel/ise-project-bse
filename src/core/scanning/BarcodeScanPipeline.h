#ifndef BARCODE_SCAN_PIPELINE_H
#define BARCODE_SCAN_PIPELINE_H

#include <string>

// Qt-free half of the scanner: turns a grayscale frame into a barcode string.
// The Qt adapter (BarcodeScannerService in src/scanning/) converts QImage
// frames to this raw format and handles throttling/signals.
//
// Keeping this in qrcore means the decode path can be tested without a camera
// or a Qt build.
class BarcodeScanPipeline
{
public:
    // grayscale: width * height bytes, 0 = black, 255 = white.
    // stride: bytes per row (may be larger than width).
    // Returns the decoded 6-digit barcode, or an empty string when no
    // decodable barcode is present.
    static std::string decode(const unsigned char *grayscale,
                              int width,
                              int height,
                              int stride);
};

#endif // BARCODE_SCAN_PIPELINE_H
