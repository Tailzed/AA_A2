#pragma once
#include "core/SeamFinder.hpp"

namespace sc {

/**
 * @brief Dynamic Programming Seam Finder.
 *
 * Implements vertical seam carving via Dynamic Programming (DP).
 * Supports both:
 *  1. Backward Energy (Avidan & Shamir 2007): accumulates precomputed gradient
 *     energy M[i][j] = e[i][j] + min(M[i-1][j-1], M[i-1][j], M[i-1][j+1]) with
 *     strict leftmost tie-breaking.
 *  2. Forward Energy (Rubinstein et al. 2008): calculates pixel differences
 *     introduced when removing a seam (C_L, C_U, C_R) to minimize visual artifacts.
 */
class DpSeamFinder final : public ISeamFinder {
public:
    /**
     * @brief Returns the identifier name of the algorithm.
     */
    std::string name() const override { return "DP"; }

    /**
     * @brief Finds the optimal vertical seam.
     *
     * Dispatches automatically:
     * - Uses backward energy when energy is precomputed or img is blank (unit tests).
     * - Uses forward energy when an actual image is provided (GUI carving).
     *
     * @param img    Input BGR/grayscale image.
     * @param energy Precomputed energy map (CV_64F).
     * @return Seam  The optimal vertical seam (column index per row) and path cost.
     */
    Seam findVertical(const cv::Mat& img, const cv::Mat& energy) override;

    /**
     * @brief Finds a vertical seam using standard backward energy DP.
     *
     * @param energy Precomputed energy map (CV_64F).
     * @return Seam  The optimal seam with leftmost tie-break.
     */
    Seam findVerticalBackward(const cv::Mat& energy);

    /**
     * @brief Finds a vertical seam using forward energy DP on image gradients.
     *
     * @param img    Input image (CV_8UC3 or CV_8UC1).
     * @return Seam  The seam minimizing forward energy insertion costs.
     */
    Seam findVerticalForward(const cv::Mat& img);
};

}  // namespace sc
