#include "test_framework.h"
#include "test_image_helpers.h"

#include "barcodeEncoder.h"
#include "scanning/BarcodeScanPipeline.h"

#include <cstddef>
#include <string>
#include <vector>

namespace {

std::string decodeImage(const GrayImage &image)
{
    return BarcodeScanPipeline::decode(
        image.pixels.data(), image.width, image.height, image.stride);
}

} // namespace

TEST_CASE(scan_pipeline_decodes_rendered_barcode)
{
    BarcodeEncoder encoder;
    const int expected = 428647;

    const GrayImage image =
        renderBarcode(encoder.encode(expected), 2, 6, 20, 20, 160);

    CHECK_EQ(decodeImage(image), std::to_string(expected));
}

TEST_CASE(scan_pipeline_decodes_under_uneven_lighting)
{
    BarcodeEncoder encoder;
    GrayImage image =
        renderBarcode(encoder.encode(123456), 2, 6, 20, 20, 160);

    // Replace the white background with a left-to-right gradient. The bars
    // stay black, but a fixed 128 threshold would lose the darker side.
    for (int y = 0; y < image.height; ++y)
    {
        for (int x = 0; x < image.width; ++x)
        {
            unsigned char &pixel =
                image.pixels[static_cast<std::size_t>(y) * image.stride + x];
            if (pixel == 255)
            {
                const int shade =
                    120 + (x * 100) / (image.width > 1 ? image.width - 1 : 1);
                pixel = static_cast<unsigned char>(shade);
            }
        }
    }

    CHECK_EQ(decodeImage(image), std::string("123456"));
}

TEST_CASE(scan_pipeline_honours_row_stride)
{
    BarcodeEncoder encoder;
    const int expected = 654321;

    const GrayImage image = addRowPadding(
        renderBarcode(encoder.encode(expected), 2, 6, 20, 20, 120), 7);

    CHECK_EQ(decodeImage(image), std::to_string(expected));
}

TEST_CASE(scan_pipeline_returns_empty_for_blank_and_invalid_input)
{
    const GrayImage blank = {200, 100, 200,
                             std::vector<unsigned char>(200 * 100, 255)};

    CHECK_EQ(decodeImage(blank), std::string(""));

    CHECK_EQ(BarcodeScanPipeline::decode(nullptr, 100, 100, 100),
             std::string(""));

    std::vector<unsigned char> tiny(10, 0);
    CHECK_EQ(BarcodeScanPipeline::decode(tiny.data(), 0, 0, 0),
             std::string(""));
    CHECK_EQ(BarcodeScanPipeline::decode(tiny.data(), 10, 1, 5),
             std::string(""));
}
