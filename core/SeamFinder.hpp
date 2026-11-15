#pragma once
#include <memory>
#include <string>

#include "core/Types.hpp"

namespace sc {

// Strategy interface: DP, Greedy and Graph-cut all implement this, so carve()
// has no per-algorithm branches and timing comparisons go through identical code.
class ISeamFinder {
public:
    virtual ~ISeamFinder() = default;
    virtual std::string name() const = 0;

    // img    : CV_8UC3 (BGR), H x W. Backward-energy finders can ignore it; a
    //          forward-energy finder needs it.
    // energy : CV_64F, H x W, non-negative, integer-valued in EnergyMode::Sobel.
    // Returns a VERTICAL seam (one column per row). Horizontal seams are handled by
    // the carver, which transposes both inputs before calling.
    //
    // Contract (checked by tests/test_contract.cpp):
    //   * the seam is valid (see isValidSeam) and deterministic
    //   * Seam::cost == sum of energy along the returned seam
    //   * ties are broken towards the LEFTMOST column
    virtual Seam findVertical(const cv::Mat& img, const cv::Mat& energy) = 0;
};

std::unique_ptr<ISeamFinder> makeFinder(Algorithm algorithm);

}  // namespace sc
