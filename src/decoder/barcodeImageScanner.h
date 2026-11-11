#ifndef BARCODE_IMAGE_SCANNER_H
#define BARCODE_IMAGE_SCANNER_H

#include <string>
#include <vector>

class BarcodeImageScanner
{
public:
    // Extracts candidate Code 39 element patterns from a grayscale image.
    // Each pixel is one byte: 0 = black, 255 = white.
    // Returns unique "0"/"1" patterns (narrow/wide) found on horizontal
    // scan lines, in scan order.
    std::vector<std::string> scan(const unsigned char *grayscale,
                                  int width,
                                  int height,
                                  int stride,
                                  unsigned char threshold = 128) const;
};

#endif
