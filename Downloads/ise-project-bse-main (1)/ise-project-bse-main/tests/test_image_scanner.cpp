#include "test_framework.h"

#include "barcodeDecoder.h"
#include "barcodeEncoder.h"
#include "barcodeImageScanner.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct GrayImage
{
    int width;
    int height;
    int stride;
    std::vector<unsigned char> pixels;
};

GrayImage renderBarcode(const std::string &pattern, int narrowWidth,
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

bool contains(const std::vector<std::string> &values, const std::string &value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

GrayImage addRowPadding(const GrayImage &source, int padding)
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

int decodeFirst(const std::vector<std::string> &candidates)
{
    BarcodeDecoder decoder;
    for (const std::string &pattern : candidates)
    {
        try
        {
            return decoder.decode(pattern);
        }
        catch (const std::exception &)
        {
        }
    }

    throw std::runtime_error("no scanner candidate could be decoded");
}

void fillRect(GrayImage &image, int x0, int y0, int width, int height,
              unsigned char value)
{
    for (int y = y0; y < y0 + height; ++y)
    {
        const std::size_t rowStart =
            static_cast<std::size_t>(y) * image.stride + x0;
        std::fill(image.pixels.begin() + rowStart,
                  image.pixels.begin() + rowStart + width, value);
    }
}

} // namespace

TEST_CASE(image_scanner_finds_pattern_at_multiple_scales)
{
    BarcodeEncoder encoder;
    BarcodeImageScanner scanner;

    const std::string pattern = encoder.encode(123456);

    struct Scale
    {
        int narrow;
        int wide;
    };

    const Scale scales[] = {{1, 3}, {2, 6}, {3, 9}, {4, 12}};
    for (const Scale &scale : scales)
    {
        const GrayImage image =
            renderBarcode(pattern, scale.narrow, scale.wide, 12, 8, 120);

        const std::vector<std::string> candidates =
            scanner.scan(image.pixels.data(), image.width, image.height,
                         image.stride);

        CHECK(contains(candidates, pattern));
    }
}

TEST_CASE(image_scanner_decodes_rendered_barcode_end_to_end)
{
    BarcodeEncoder encoder;
    BarcodeImageScanner scanner;

    const int expected = 999999;
    const GrayImage image =
        renderBarcode(encoder.encode(expected), 2, 6, 20, 20, 160);

    const std::vector<std::string> candidates =
        scanner.scan(image.pixels.data(), image.width, image.height,
                     image.stride);

    CHECK(!candidates.empty());
    CHECK_EQ(decodeFirst(candidates), expected);
}

TEST_CASE(image_scanner_finds_barcode_inside_larger_canvas)
{
    BarcodeEncoder encoder;
    BarcodeImageScanner scanner;

    const std::string pattern = encoder.encode(654321);
    const GrayImage image = renderBarcode(pattern, 2, 6, 30, 400, 100);

    const std::vector<std::string> candidates =
        scanner.scan(image.pixels.data(), image.width, image.height,
                     image.stride);

    CHECK(contains(candidates, pattern));
}

TEST_CASE(image_scanner_finds_barcode_between_other_dark_objects)
{
    BarcodeEncoder encoder;
    BarcodeImageScanner scanner;

    const std::string pattern = encoder.encode(246810);
    const int margin = 80;
    GrayImage image = renderBarcode(pattern, 2, 6, margin, 20, 120);

    // Dark objects on the same rows as the barcode, like a desk or a hand in
    // a camera frame. The old whole-row scan counted their edges as barcode
    // elements and gave up.
    fillRect(image, 0, 0, 40, image.height, 0);
    fillRect(image, image.width - 40, 0, 40, image.height, 0);

    const std::vector<std::string> candidates =
        scanner.scan(image.pixels.data(), image.width, image.height,
                     image.stride);

    CHECK(contains(candidates, pattern));
    CHECK_EQ(decodeFirst(candidates), 246810);
}

TEST_CASE(image_scanner_ignores_stride_padding)
{
    BarcodeEncoder encoder;
    BarcodeImageScanner scanner;

    const std::string pattern = encoder.encode(123456);
    const GrayImage image =
        addRowPadding(renderBarcode(pattern, 2, 6, 20, 20, 120), 7);

    const std::vector<std::string> candidates =
        scanner.scan(image.pixels.data(), image.width, image.height,
                     image.stride);

    CHECK(contains(candidates, pattern));
}

TEST_CASE(image_scanner_returns_no_candidates_for_blank_image)
{
    BarcodeImageScanner scanner;

    const GrayImage blank = {200, 100, 200,
                             std::vector<unsigned char>(200 * 100, 255)};

    const std::vector<std::string> candidates =
        scanner.scan(blank.pixels.data(), blank.width, blank.height,
                     blank.stride);

    CHECK(candidates.empty());
}

TEST_CASE(image_scanner_returns_no_candidates_for_solid_black_image)
{
    BarcodeImageScanner scanner;

    const std::vector<unsigned char> black(200 * 100, 0);

    CHECK(scanner.scan(black.data(), 200, 100, 200).empty());
}

TEST_CASE(image_scanner_rejects_invalid_input)
{
    BarcodeImageScanner scanner;

    CHECK(scanner.scan(nullptr, 100, 100, 100).empty());
    CHECK(scanner.scan(std::vector<unsigned char>(10, 0).data(), 0, 0, 0).empty());
    CHECK(scanner.scan(std::vector<unsigned char>(10, 0).data(), 10, 1, 5).empty());
}
