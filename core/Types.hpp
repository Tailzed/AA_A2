#pragma once
// Shared value types for the seam carving core. No GUI dependencies (OpenCV only).

#include <functional>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace sc {

// A vertical seam: one column index per image row.
struct Seam {
    std::vector<int> cols;  // cols[i] = column removed in row i; size == image height
    double cost = 0.0;      // sum of energy along the seam, AS REPORTED BY THE FINDER
                            // (tests never trust this; they recompute with seamCost())
};

enum class Algorithm { DP, Greedy, GraphCut };

enum class EnergyMode {
    Sobel,     // |gx| + |gy|, integer-valued (exact == comparisons are safe)
    Gradient   // sqrt(gx^2 + gy^2), NOT integer-valued (tests need a tolerance)
};

struct CarveOptions {
    EnergyMode energy = EnergyMode::Sobel;
};

// Called after every removed seam. `done` counts 1..total. The seam's column
// indices refer to the image AS IT WAS when the seam was found (already shrunk,
// and transposed during the horizontal phase). Return false to cancel.
using ProgressFn = std::function<bool(int done, int total, const Seam& seam)>;

struct CarveResult {
    cv::Mat image;              // the resized image (same type as the input)
    int seamsRemoved = 0;
    bool cancelled = false;
    double elapsedMs = 0.0;     // whole carve() call, energy computation included
    cv::Mat1i removedOrder;     // original-size map: 0 = kept, k > 0 = removed by k-th seam
};

}  // namespace sc
