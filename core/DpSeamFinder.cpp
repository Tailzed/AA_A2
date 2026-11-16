#include "core/DpSeamFinder.hpp"

#include "core/SeamOps.hpp"
#include <cstdint>

namespace sc {
    // STUB: returns the middle column so the GUI runs end to end.
    // Replace the body with the real dynamic programming:
    //   M[0][j] = e[0][j]
    //   M[i][j] = e[i][j] + min(M[i-1][j-1], M[i-1][j], M[i-1][j+1])   (skip out-of-range parents)
    //   start at argmin_j M[H-1][j], backtrack to row 0 choosing the minimising parent.
    // Remember: ties -> leftmost column, and fill Seam::cost with the real path cost.
    // The [optimality] and [contract] tests stay red until this is done.
    Seam DpSeamFinder::findVertical(const cv::Mat& img, const cv::Mat& energy) {
        Seam ret{};

        cv::Mat working = energy.clone();
CV_Assert(energy.type() == CV_64F);
        cv::copyMakeBorder(
            working, 
            working, 
            0, 0, 1, 1,     // only pad left and right 
            cv::BORDER_CONSTANT,
            std::numeric_limits<double>::infinity()
        );

        
        for(int32_t row{1}; row < working.rows; ++row){
            // working.at<float>(row, working.cols-1) = std::numeric_limits::float::max();
            // working.at<float>(row, 0) = std::numeric_limits::float::max();

            for(int32_t col{1}; col < working.cols-1; ++col){
                working.at<double>(row, col) += (
                    std::min(
                        working.at<double>(row-1, col-1),
                        std::min(
                            working.at<double>(row-1, col),
                            working.at<double>(row-1, col+1)
                        )
                    )
                );
            }
        }

        // cv::Rect roi(left, top, energy.cols, energy.rows);
        // working = working(roi).clone();

        int32_t col{1};
        for(int32_t i{2}; i < working.cols-1; ++i){
            col = working.at<double>(working.rows-1, col) <= working.at<double>(working.rows-1, i) ? col : i;
        }
        ret.cols.push_back(col-1); 
        ret.cost = working.at<double>(working.rows-1, col);

        auto cmp_p = [](std::pair<int, double> const& a, std::pair<int, double> const& b){
            return a.second < b.second;
        };

        for(int32_t row{working.rows-2}; row >= 0; --row){
            std::pair<int, double> 
                L{col-1, working.at<double>(row, col-1)},
                C{col, working.at<double>(row, col)},
                R{col+1, working.at<double>(row, col+1)}
            ;
            ret.cols.push_back((col = std::min(L, std::min(C, R, cmp_p), cmp_p).first)-1);
        }
        std::reverse(ret.cols.begin(), ret.cols.end());

        return ret;
    }

}  // namespace sc
