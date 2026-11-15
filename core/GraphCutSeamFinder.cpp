#include "core/GraphCutSeamFinder.hpp"

#include "core/SeamOps.hpp"

namespace sc {

// STUB: returns the middle column so the GUI runs end to end.
// Real version: build the s-t graph described in assignment item 4 (see the header
// for the padding note), run Edmonds-Karp, BFS from the source in the residual graph,
// and in each row take the rightmost pixel in S as the seam pixel.
Seam GraphCutSeamFinder::findVertical(const cv::Mat& /*img*/, const cv::Mat& energy) {
    return middleColumnSeam(energy);
}

}  // namespace sc
