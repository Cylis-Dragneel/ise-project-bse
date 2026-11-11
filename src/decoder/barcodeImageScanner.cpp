#include "barcodeImageScanner.h"

#include <algorithm>
#include <cstddef>

namespace {

const std::size_t ELEMENTS_PER_BARCODE = 79;
const std::size_t WIDE_ELEMENTS_PER_BARCODE = 24; // 3 wide elements per character
const std::size_t MAX_CANDIDATES = 16;

std::string classifyRuns(const std::vector<int> &runs)
{
    if (runs.size() != ELEMENTS_PER_BARCODE)
    {
        return std::string();
    }

    // Code 39 always has exactly 24 wide elements in a 79-element barcode.
    std::vector<int> sorted = runs;
    std::sort(sorted.begin(), sorted.end());
    const int wideThreshold =
        sorted[sorted.size() - WIDE_ELEMENTS_PER_BARCODE];

    std::string pattern;
    pattern.reserve(runs.size());

    for (int run : runs)
    {
        pattern.push_back(run >= wideThreshold ? '1' : '0');
    }

    return pattern;
}

} // namespace

std::vector<std::string> BarcodeImageScanner::scan(
    const unsigned char *grayscale, int width, int height, int stride,
    unsigned char threshold) const
{
    std::vector<std::string> candidates;

    if (grayscale == nullptr || width <= 0 || height <= 0 || stride < width)
    {
        return candidates;
    }

    for (int y = 0; y < height; ++y)
    {
        const unsigned char *row =
            grayscale + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride);

        // Ignore the quiet zone around the barcode.
        int first = 0;
        while (first < width && row[first] >= threshold)
        {
            ++first;
        }
        if (first == width)
        {
            continue;
        }

        int last = width - 1;
        while (last > first && row[last] >= threshold)
        {
            --last;
        }

        std::vector<int> runs;
        runs.reserve(ELEMENTS_PER_BARCODE);

        bool currentIsBlack = true;
        int runLength = 0;
        for (int x = first; x <= last; ++x)
        {
            const bool isBlack = row[x] < threshold;
            if (isBlack == currentIsBlack)
            {
                ++runLength;
            }
            else
            {
                runs.push_back(runLength);
                currentIsBlack = isBlack;
                runLength = 1;
            }
        }
        runs.push_back(runLength);

        const std::string pattern = classifyRuns(runs);
        if (pattern.empty())
        {
            continue;
        }

        if (std::find(candidates.begin(), candidates.end(), pattern) ==
            candidates.end())
        {
            candidates.push_back(pattern);
            if (candidates.size() >= MAX_CANDIDATES)
            {
                break;
            }
        }
    }

    return candidates;
}
