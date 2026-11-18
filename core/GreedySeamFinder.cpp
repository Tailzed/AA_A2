#include "core/GreedySeamFinder.hpp"

#include <limits>

namespace sc {

// Row-by-row minimum: start at the cheapest pixel of row 0, then step to the
// cheapest of the three connected pixels below. Strict '<' scanned left to right
// keeps the leftmost column on a tie. Seam::cost is the sum along that path.
Seam GreedySeamFinder::findVertical(const cv::Mat& /*img*/, const cv::Mat& energy) {
    Seam seam;
    const int H = energy.rows;
    const int W = energy.cols;
    if (H <= 0 || W <= 0) return seam;

    seam.cols.resize(H);
    seam.cost = 0.0;

    if (W == 1) {
        for (int r = 0; r < H; ++r) {
            seam.cols[r] = 0;
            seam.cost += energy.at<double>(r, 0);
        }
        return seam;
    }

    int bestCol = 0;
    double bestVal = energy.at<double>(0, 0);
    for (int c = 1; c < W; ++c) {
        const double val = energy.at<double>(0, c);
        if (val < bestVal) {
            bestVal = val;
            bestCol = c;
        }
    }
    seam.cols[0] = bestCol;
    seam.cost += bestVal;

    for (int r = 1; r < H; ++r) {
        const int prevCol = seam.cols[r - 1];
        int nextCol = prevCol;
        double minNeighborVal = std::numeric_limits<double>::infinity();
        for (int delta = -1; delta <= 1; ++delta) {
            const int c = prevCol + delta;
            if (c < 0 || c >= W) continue;
            const double val = energy.at<double>(r, c);
            if (val < minNeighborVal) {
                minNeighborVal = val;
                nextCol = c;
            }
        }
        seam.cols[r] = nextCol;
        seam.cost += minNeighborVal;
    }

    return seam;
}

}  // namespace sc
