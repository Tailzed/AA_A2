#include "core/DpSeamFinder.hpp"

#include "core/SeamOps.hpp"

namespace sc {

// STUB: returns the middle column so the GUI runs end to end.
// Replace the body with the real dynamic programming:
//   M[0][j] = e[0][j]
//   M[i][j] = e[i][j] + min(M[i-1][j-1], M[i-1][j], M[i-1][j+1])   (skip out-of-range parents)
//   start at argmin_j M[H-1][j], backtrack to row 0 choosing the minimising parent.
// Remember: ties -> leftmost column, and fill Seam::cost with the real path cost.
// The [optimality] and [contract] tests stay red until this is done.
Seam DpSeamFinder::findVertical(const cv::Mat& /*img*/, const cv::Mat& energy) {
    return middleColumnSeam(energy);
}

}  // namespace sc
