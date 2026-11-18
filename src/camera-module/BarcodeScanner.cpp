#include "BarcodeScanner.h"
#include "Decoder/barcodeDecoder.h"
#include <QImage>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {

constexpr int kElements   = 79;  // 8 chars * 9 elements + 7 inter-character gaps
constexpr int kWideCount  = 24;  // 8 chars * 3 wide elements each
constexpr int kMinContrast = 40; // ignore rows that are basically flat

struct Run {
    bool dark;
    int  length;
};

// Average a horizontal band of rows into one 1-D brightness profile (reduces noise).
std::vector<int> makeProfile(const QImage &gray, int centerY, int halfBand)
{
    const int w = gray.width();
    const int h = gray.height();
    std::vector<int> sum(w, 0);
    int rows = 0;
    for (int y = centerY - halfBand; y <= centerY + halfBand; ++y) {
        if (y < 0 || y >= h) continue;
        const uchar *line = gray.constScanLine(y);
        for (int x = 0; x < w; ++x) sum[x] += line[x];
        ++rows;
    }
    if (rows == 0) return {};
    for (int &v : sum) v /= rows;
    return sum;
}

// Threshold the profile and run-length encode it into alternating dark/light runs.
std::vector<Run> toRuns(const std::vector<int> &profile)
{
    std::vector<Run> runs;
    if (profile.size() < 10) return runs;

    // Robust min/max (2nd and 98th percentile) so a few noisy pixels don't skew the threshold
    std::vector<int> sorted(profile);
    std::sort(sorted.begin(), sorted.end());
    int lo = sorted[sorted.size() * 2 / 100];
    int hi = sorted[sorted.size() * 98 / 100];
    if (hi - lo < kMinContrast) return runs;
    int threshold = (lo + hi) / 2;

    bool current = profile[0] < threshold;
    int  length  = 1;
    for (size_t i = 1; i < profile.size(); ++i) {
        bool dark = profile[i] < threshold;
        if (dark == current) {
            ++length;
        } else {
            runs.push_back({current, length});
            current = dark;
            length  = 1;
        }
    }
    runs.push_back({current, length});
    return runs;
}

// Turn 79 consecutive run lengths into a '0' (narrow) / '1' (wide) string.
// We know exactly 24 of the 79 elements are wide, so the narrow/wide split is
// found from the data itself and works at any scale or camera distance.
std::string classify(const std::vector<Run> &runs, size_t first)
{
    std::vector<int> widths;
    widths.reserve(kElements);
    for (int i = 0; i < kElements; ++i) widths.push_back(runs[first + i].length);

    std::vector<int> sorted(widths);
    std::sort(sorted.begin(), sorted.end());

    const int narrowCount = kElements - kWideCount;
    double narrowAvg = 0, wideAvg = 0;
    for (int i = 0; i < narrowCount; ++i)  narrowAvg += sorted[i];
    for (int i = narrowCount; i < kElements; ++i) wideAvg += sorted[i];
    narrowAvg /= narrowCount;
    wideAvg   /= kWideCount;

    // Wide should be clearly bigger than narrow (Code 39 uses roughly 2:1 to 3:1)
    if (wideAvg < narrowAvg * 1.5) return "";

    const double threshold = (narrowAvg + wideAvg) / 2.0;
    std::string out;
    out.reserve(kElements);
    for (int w : widths) out += (w > threshold ? '1' : '0');
    return out;
}

// Scan one row band. Returns a decodable pattern if found, otherwise a
// well-formed-but-undecodable candidate (so the UI can show the decoder's error).
std::string scanBand(const QImage &gray, int centerY, std::string &fallback)
{
    std::vector<Run> runs = toRuns(makeProfile(gray, centerY, 2));
    if (runs.size() < static_cast<size_t>(kElements)) return "";

    BarcodeDecoder decoder;

    // A barcode begins with a dark bar and has exactly 79 runs.
    for (size_t s = 0; s + kElements <= runs.size(); ++s) {
        if (!runs[s].dark) continue;
        // 79 runs starting with a bar must also end with a bar
        if (!runs[s + kElements - 1].dark) continue;

        std::string candidate = classify(runs, s);
        if (candidate.empty()) continue;

        try {
            decoder.decode(candidate);   // validates start/stop, gaps, digits
            return candidate;            // success
        } catch (...) {
            if (fallback.empty()) fallback = candidate;
        }
    }
    return "";
}

} // namespace

std::string BarcodeScanner::extractBarcodeString(const QImage &image)
{
    if (image.isNull()) return "";

    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    if (gray.isNull() || gray.width() < kElements || gray.height() == 0) return "";

    // Try the middle first, then fan out: the barcode may not be exactly centered.
    const double fractions[] = {0.50, 0.42, 0.58, 0.34, 0.66, 0.26, 0.74, 0.18, 0.82};

    std::string fallback;
    for (double f : fractions) {
        std::string result = scanBand(gray, static_cast<int>(gray.height() * f), fallback);
        if (!result.empty()) return result;
    }
    return fallback; // empty if nothing barcode-like was found
}