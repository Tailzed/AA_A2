#include "core/DpSeamFinder.hpp"

#include "core/SeamOps.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <opencv2/imgproc.hpp>

namespace sc {

// =========================================================================
// Backward Energy Dynamic Programming (Avidan & Shamir, 2007)
// =========================================================================
Seam DpSeamFinder::findVerticalBackward(const cv::Mat& energy) {
    Seam ret{};
    if (energy.empty() || energy.rows <= 0 || energy.cols <= 0) return ret;

    cv::Mat working = energy.clone();

    // Pad left and right with infinity to handle image boundaries cleanly
    cv::copyMakeBorder(
        working, 
        working, 
        0, 0, 1, 1,
        cv::BORDER_CONSTANT,
        std::numeric_limits<double>::infinity()
    );

    // 1. Accumulate cumulative minimum energy table:
    //    M[i][j] = e[i][j] + min(M[i-1][j-1], M[i-1][j], M[i-1][j+1])
    for (int32_t row{1}; row < working.rows; ++row) {
        for (int32_t col{1}; col < working.cols - 1; ++col) {
            working.at<double>(row, col) += (
                std::min(
                    working.at<double>(row - 1, col - 1),
                    std::min(
                        working.at<double>(row - 1, col),
                        working.at<double>(row - 1, col + 1)
                    )
                )
            );
        }
    }

    // 2. Find the minimum in the bottom row (scanned left-to-right, ties pick leftmost)
    int32_t col{1};
    for (int32_t i{2}; i < working.cols - 1; ++i) {
        col = working.at<double>(working.rows - 1, col) <= working.at<double>(working.rows - 1, i) ? col : i;
    }
    ret.cols.push_back(col - 1); 
    ret.cost = working.at<double>(working.rows - 1, col);

    // Leftmost tie-break comparator
    auto cmp_p = [](std::pair<int, double> const& a, std::pair<int, double> const& b) {
        return a.second < b.second;
    };

    // 3. Backtrack upwards to row 0, breaking ties towards the leftmost column (L > C > R)
    for (int32_t row{working.rows - 2}; row >= 0; --row) {
        std::pair<int, double> 
            L{col - 1, working.at<double>(row, col - 1)},
            C{col,     working.at<double>(row, col)},
            R{col + 1, working.at<double>(row, col + 1)}
        ;
        ret.cols.push_back((col = std::min(L, std::min(C, R, cmp_p), cmp_p).first) - 1);
    }
    std::reverse(ret.cols.begin(), ret.cols.end());

    return ret;
}

// =========================================================================
// Forward Energy Dynamic Programming (Rubinstein et al., 2008)
// =========================================================================
Seam DpSeamFinder::findVerticalForward(const cv::Mat& img) {
    Seam ret{};
    if (img.empty() || img.rows <= 0 || img.cols <= 0) return ret;

    cv::Mat I{}, working{}, gray{};
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    gray.convertTo(I, CV_32F);

    std::vector<int> parent_mtx_back(img.rows * img.cols);
    working = cv::Mat::zeros(I.size(), I.type());

    // Pad image with replicate border for horizontal difference computation
    cv::copyMakeBorder(
        I, 
        I, 
        0, 0, 1, 1,
        cv::BORDER_REPLICATE
    );

    // Pad cost matrix with infinity on left and right borders
    cv::copyMakeBorder(
        working, 
        working, 
        0, 0, 1, 1,
        cv::BORDER_CONSTANT,
        std::numeric_limits<float>::infinity()
    );

    auto cmp_p = [](std::pair<int, float> const& a, std::pair<int, float> const& b) {
        return a.second < b.second;
    };

    // Forward energy dynamic programming table accumulation
    // Evaluates insertion costs C_U (center), C_L (left), C_R (right)
    for (int32_t row{1}; row < working.rows; ++row) {
        for (int32_t col{1}; col < working.cols - 1; ++col) {
            std::pair<int, float> 
                OUT{
                    std::min(
                        std::pair<int, float>{0, working.at<float>(row - 1, col) + std::abs(I.at<float>(row, col - 1) - I.at<float>(row, col + 1))},
                        std::min(
                            std::pair<int, float>{-1, working.at<float>(row - 1, col - 1) + std::abs(I.at<float>(row, col - 1) - I.at<float>(row, col + 1)) + std::abs(I.at<float>(row, col - 1) - I.at<float>(row - 1, col))},
                            std::pair<int, float>{+1, working.at<float>(row - 1, col + 1) + std::abs(I.at<float>(row, col - 1) - I.at<float>(row, col + 1)) + std::abs(I.at<float>(row, col + 1) - I.at<float>(row - 1, col))},
                            cmp_p
                        ),
                        cmp_p
                    )
                }
            ;
            parent_mtx_back[row * (working.cols - 2) + (col - 1)] = OUT.first;
            working.at<float>(row, col) = OUT.second;
        }
    }

    // Find the minimum in the bottom row
    int32_t col{1};
    for (int32_t i{2}; i < working.cols - 1; ++i) {
        col = working.at<float>(working.rows - 1, col) <= working.at<float>(working.rows - 1, i) ? col : i;
    }
    ret.cols.push_back(col - 1); 
    ret.cost = working.at<float>(working.rows - 1, col);

    // Backtrack using the parent matrix
    for (int32_t row{working.rows - 1}; row > 0; --row) {
        ret.cols.push_back(ret.cols.back() + parent_mtx_back[row * (working.cols - 2) + ret.cols.back()]);
    }
    std::reverse(ret.cols.begin(), ret.cols.end());
    assert(ret.cols.size() == static_cast<size_t>(img.rows));
    return ret;
}

// =========================================================================
// Main Interface Dispatcher
// =========================================================================
Seam DpSeamFinder::findVertical(const cv::Mat& img, const cv::Mat& energy) {
    if (img.empty() || cv::countNonZero(img.reshape(1)) == 0) {
        return findVerticalBackward(energy);
    }
    return findVerticalForward(img);
}

}  // namespace sc
