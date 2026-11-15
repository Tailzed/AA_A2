#include "core/GreedySeamFinder.hpp"

#include "core/SeamOps.hpp"

namespace sc {

// STUB: returns the middle column so the GUI runs end to end.
// Real version: start at the minimum-energy pixel of row 0, then in every next row
// move to the cheapest of the three pixels below (col-1, col, col+1, within bounds),
// ties -> leftmost. Fill Seam::cost with the real path cost.
// tests/test_optimality.cpp contains a 3x4 "greedy trap" with hand-checked costs.
Seam GreedySeamFinder::findVertical(const cv::Mat& /*img*/, const cv::Mat& energy) {
    return middleColumnSeam(energy);
}

}  // namespace sc
