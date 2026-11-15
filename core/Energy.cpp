#include "core/Energy.hpp"

#include <stdexcept>

#include <opencv2/imgproc.hpp>

namespace sc {

cv::Mat computeEnergy(const cv::Mat& img, EnergyMode mode) {
    if (img.empty()) throw std::invalid_argument("computeEnergy: empty image");
    if (img.depth() != CV_8U) throw std::invalid_argument("computeEnergy: expected an 8-bit image");

    cv::Mat gray;
    switch (img.channels()) {
        case 1: gray = img; break;
        case 3: cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY); break;
        case 4: cv::cvtColor(img, gray, cv::COLOR_BGRA2GRAY); break;
        default: throw std::invalid_argument("computeEnergy: unsupported channel count");
    }

    // Sobel of integer pixels with integer kernel weights is an exact integer,
    // and doubles represent such integers exactly.
    cv::Mat gx, gy;
    cv::Sobel(gray, gx, CV_64F, 1, 0, 3, 1.0, 0.0, cv::BORDER_REPLICATE);
    cv::Sobel(gray, gy, CV_64F, 0, 1, 3, 1.0, 0.0, cv::BORDER_REPLICATE);

    cv::Mat energy;
    if (mode == EnergyMode::Sobel) {
        energy = cv::abs(gx) + cv::abs(gy);
    } else {
        cv::magnitude(gx, gy, energy);
    }
    return energy;  // CV_64F
}

}  // namespace sc
