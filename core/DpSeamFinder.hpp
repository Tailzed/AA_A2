#pragma once
#include "core/SeamFinder.hpp"

namespace sc {

// >>> YOURS TO IMPLEMENT (assignment item 1b) <<<
class DpSeamFinder final : public ISeamFinder {
public:
    std::string name() const override { return "DP"; }
    Seam findVertical(const cv::Mat& img, const cv::Mat& energy) override;
};

}  // namespace sc
