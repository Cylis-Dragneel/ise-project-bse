#include "barcodeImageScanner.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace {

constexpr std::size_t ELEMENTS_PER_BARCODE = 79;
constexpr std::size_t WIDE_ELEMENTS_PER_BARCODE = 24; // 3 wide elements per character
constexpr std::size_t MAX_CANDIDATES = 64;

// Every Code 39 barcode starts with the same start/stop character. Checking
// it before accepting a window throws away almost all noise candidates.
constexpr char START_STOP_PATTERN[] = "010010100";
constexpr std::size_t START_STOP_LENGTH = 9;

// Classifies a single 79-element window of the row, using the window's own
// 24 widest runs as the wide/narrow cutoff. Code 39 always has exactly 24
// wide elements, so this works regardless of scale or perspective.
std::string classifyWindow(const std::vector<int> &runs, std::size_t offset)
{
    std::array<int, ELEMENTS_PER_BARCODE> window;
    std::copy_n(runs.begin() + static_cast<std::ptrdiff_t>(offset),
                ELEMENTS_PER_BARCODE, window.begin());

    std::array<int, ELEMENTS_PER_BARCODE> sorted = window;
    std::nth_element(sorted.begin(),
                     sorted.begin() +
                         static_cast<std::ptrdiff_t>(ELEMENTS_PER_BARCODE -
                                                     WIDE_ELEMENTS_PER_BARCODE),
                     sorted.end());

    const int wideThreshold =
        sorted[ELEMENTS_PER_BARCODE - WIDE_ELEMENTS_PER_BARCODE];

    std::string pattern;
    pattern.reserve(ELEMENTS_PER_BARCODE);

    for (int run : window)
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

        std::vector<int> runs;
        runs.reserve(ELEMENTS_PER_BARCODE);

        bool currentIsBlack = true;
        int runLength = 0;
        for (int x = first; x < width; ++x)
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

        if (runs.size() < ELEMENTS_PER_BARCODE)
        {
            continue;
        }

        // A row can contain other dark objects around the barcode, so try
        // every possible 79-element window instead of assuming the whole
        // row is one barcode. Windows must start on a black run (even index).
        for (std::size_t offset = 0;
             offset + ELEMENTS_PER_BARCODE <= runs.size(); offset += 2)
        {
            const std::string pattern = classifyWindow(runs, offset);
            if (pattern.compare(0, START_STOP_LENGTH, START_STOP_PATTERN) != 0)
            {
                continue;
            }

            if (std::find(candidates.begin(), candidates.end(), pattern) ==
                candidates.end())
            {
                candidates.push_back(pattern);
                if (candidates.size() >= MAX_CANDIDATES)
                {
                    return candidates;
                }
            }
        }
    }

    return candidates;
}
