#include "BarcodeScanPipeline.h"

#include "barcodeDecoder.h"
#include "barcodeImageScanner.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <string>
#include <vector>

namespace {

// Otsu's method: picks the threshold that best separates the dark bars from
// the light background. Camera frames are rarely evenly lit, so a fixed 128
// is unreliable there.
int otsuThreshold(const unsigned char *grayscale, int width, int height,
                  int stride)
{
    std::array<std::size_t, 256> histogram{};
    for (int y = 0; y < height; ++y)
    {
        const unsigned char *row =
            grayscale + static_cast<std::size_t>(y) *
                            static_cast<std::size_t>(stride);
        for (int x = 0; x < width; ++x)
        {
            ++histogram[row[x]];
        }
    }

    const std::size_t total = static_cast<std::size_t>(width) *
                              static_cast<std::size_t>(height);
    if (total == 0)
    {
        return 128;
    }

    double sum = 0.0;
    for (int i = 0; i < 256; ++i)
    {
        sum += static_cast<double>(i) * static_cast<double>(histogram[i]);
    }

    double sumBackground = 0.0;
    std::size_t weightBackground = 0;
    double bestVariance = -1.0;
    int bestThreshold = 128;

    for (int t = 0; t < 256; ++t)
    {
        weightBackground += histogram[t];
        if (weightBackground == 0)
        {
            continue;
        }

        const std::size_t weightForeground = total - weightBackground;
        if (weightForeground == 0)
        {
            break;
        }

        sumBackground += static_cast<double>(t) * static_cast<double>(histogram[t]);
        const double meanBackground =
            sumBackground / static_cast<double>(weightBackground);
        const double meanForeground =
            (sum - sumBackground) / static_cast<double>(weightForeground);
        const double difference = meanBackground - meanForeground;
        const double variance = static_cast<double>(weightBackground) *
                                static_cast<double>(weightForeground) *
                                difference * difference;

        if (variance > bestVariance)
        {
            bestVariance = variance;
            bestThreshold = t;
        }
    }

    return bestThreshold;
}

} // namespace

std::string BarcodeScanPipeline::decode(const unsigned char *grayscale,
                                        int width,
                                        int height,
                                        int stride)
{
    if (grayscale == nullptr || width <= 0 || height <= 0 || stride < width)
    {
        return {};
    }

    BarcodeImageScanner scanner;
    BarcodeDecoder decoder;

    // Try the frame's own contrast first, then the fixed threshold that clean,
    // evenly lit uploads use.
    const int adaptive =
        std::clamp(otsuThreshold(grayscale, width, height, stride), 1, 254);

    std::vector<std::string> candidates = scanner.scan(
        grayscale, width, height, stride,
        static_cast<unsigned char>(adaptive));

    if (adaptive != 128)
    {
        const std::vector<std::string> fallback =
            scanner.scan(grayscale, width, height, stride);

        for (const std::string &pattern : fallback)
        {
            if (std::find(candidates.begin(), candidates.end(), pattern) ==
                candidates.end())
            {
                candidates.push_back(pattern);
            }
        }
    }

    for (const std::string &pattern : candidates)
    {
        try
        {
            return std::to_string(decoder.decode(pattern));
        }
        catch (const std::exception &)
        {
            // This scan line did not contain a decodable barcode. Try the next.
        }
    }

    return {};
}
