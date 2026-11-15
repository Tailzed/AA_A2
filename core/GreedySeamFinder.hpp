#pragma once
#include "core/SeamFinder.hpp"

namespace sc {

// >>> YOURS TO IMPLEMENT (assignment item 3) <<<
class GreedySeamFinder final : public ISeamFinder {
public:
    std::string name() const override { return "Greedy"; }
    Seam findVertical(const cv::Mat& img, const cv::Mat& energy) override;
};

}  // namespace sc
