#pragma once
#include <climits>

#include "core/Types.hpp"

namespace sc {

// A seam is valid when it has exactly `rows` entries, every column is in
// [0, width) and adjacent rows differ by at most one column.
bool isValidSeam(const std::vector<int>& cols, int rows, int width);
inline bool isValidSeam(const Seam& s, int rows, int width) { return isValidSeam(s.cols, rows, width); }

// Sum of energy[i][cols[i]] over all rows. `energy` must be CV_64F.
// This is the single source of truth for seam cost: the tests use it instead
// of trusting Seam::cost.
double seamCost(const cv::Mat& energy, const std::vector<int>& cols);
inline double seamCost(const cv::Mat& energy, const Seam& s) { return seamCost(energy, s.cols); }

// Straight seam down the middle column. Used by the stub finders and the test mock.
Seam middleColumnSeam(const cv::Mat& energy);

// Returns a copy of `img` with one pixel removed per row (works for any depth/channels).
cv::Mat removeVerticalSeam(const cv::Mat& img, const std::vector<int>& cols);

cv::Mat transposeImage(const cv::Mat& img);

// Paints removed pixels on a copy of the original (BGR). Only seams with
// label <= upTo are drawn, which gives step-by-step visualisation.
cv::Mat overlaySeams(const cv::Mat& original, const cv::Mat1i& removedOrder,
                     int upTo = INT_MAX, const cv::Scalar& color = cv::Scalar(0, 0, 255));

}  // namespace sc
