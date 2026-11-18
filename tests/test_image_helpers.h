#ifndef TEST_IMAGE_HELPERS_H
#define TEST_IMAGE_HELPERS_H

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

// Shared helpers for the decoder/scanner tests: build a grayscale image that
// contains a rendered Code 39 pattern, without pulling in Qt.
struct GrayImage
{
    int width;
    int height;
    int stride;
    std::vector<unsigned char> pixels;
};

inline GrayImage renderBarcode(const std::string &pattern, int narrowWidth,
                               int wideWidth, int marginX, int marginY,
                               int barHeight)
{
    int barcodeWidth = 0;
    for (char element : pattern)
    {
        barcodeWidth += element == '1' ? wideWidth : narrowWidth;
    }

    GrayImage image;
    image.width = barcodeWidth + 2 * marginX;
    image.height = barHeight + 2 * marginY;
    image.stride = image.width;
    image.pixels.assign(static_cast<std::size_t>(image.width) *
                            static_cast<std::size_t>(image.height),
                        255);

    int x = marginX;
    for (std::size_t i = 0; i < pattern.size(); ++i)
    {
        const int elementWidth = pattern[i] == '1' ? wideWidth : narrowWidth;
        if (i % 2 == 0)
        {
            for (int y = marginY; y < marginY + barHeight; ++y)
            {
                const std::size_t rowStart =
                    static_cast<std::size_t>(y) * image.stride + x;
                std::fill(image.pixels.begin() + rowStart,
                          image.pixels.begin() + rowStart + elementWidth, 0);
            }
        }
        x += elementWidth;
    }

    return image;
}

inline GrayImage addRowPadding(const GrayImage &source, int padding)
{
    GrayImage padded;
    padded.width = source.width;
    padded.height = source.height;
    padded.stride = source.width + padding;
    padded.pixels.assign(static_cast<std::size_t>(padded.stride) *
                             static_cast<std::size_t>(padded.height),
                         0);

    for (int y = 0; y < source.height; ++y)
    {
        const std::size_t sourceOffset =
            static_cast<std::size_t>(y) * source.stride;
        const std::size_t paddedOffset =
            static_cast<std::size_t>(y) * padded.stride;
        std::copy(source.pixels.begin() + sourceOffset,
                  source.pixels.begin() + sourceOffset + source.width,
                  padded.pixels.begin() + paddedOffset);
    }

    return padded;
}

#endif // TEST_IMAGE_HELPERS_H
