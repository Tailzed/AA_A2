#pragma once
#include "core/Types.hpp"

namespace sc {

// Energy map of an 8-bit image (1, 3 = BGR or 4 = BGRA channels).
// Returns CV_64F, same size as `img`, all values >= 0.
//
// EnergyMode::Sobel    : |gx| + |gy| with a 3x3 Sobel kernel on the grayscale image,
//                        BORDER_REPLICATE. Always a whole number in [0, 2040], so
//                        sums along a seam are exact in double precision.
// EnergyMode::Gradient : sqrt(gx^2 + gy^2). Not integer-valued.
cv::Mat computeEnergy(const cv::Mat& img, EnergyMode mode = EnergyMode::Sobel);

}  // namespace sc
