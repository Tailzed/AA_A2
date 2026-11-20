#pragma once
#include "core/SeamFinder.hpp"

namespace sc {
//
// Notes for the implementation:
//  * Use int64_t capacities (llround of the energy). "Infinity" = a large int64
//    constant; int32 is not enough (finite capacity can reach ~2040*H*W).
//  * The spec wires the LAST column to the sink, so that column is always on the
//    sink side and the seam (rightmost pixel in S) can never be in the last column,
//    while DP can use it. To get the true optimum, pad the graph with one dummy
//    column on the right: edge [i][n-1] -> [i][n] with capacity e[i][n-1], and wire
//    the dummy column to the sink instead. Otherwise tests/test_optimality.cpp test 22
//    (graph-cut cost == brute-force minimum) can fail through no fault of your max-flow.
class GraphCutSeamFinder final : public ISeamFinder {
public:
    std::string name() const override { return "GraphCut"; }
    Seam findVertical(const cv::Mat& img, const cv::Mat& energy) override;
};

}  // namespace sc
