#include "core/DpSeamFinder.hpp"

#include "core/SeamOps.hpp"
#include <cstdint>
#include <opencv2/imgproc.hpp>

namespace sc {
    // STUB: someone else is implementing DP; placeholder stub:
    // Seam DpSeamFinder::findVertical(const cv::Mat& /*img*/, const cv::Mat& energy) {
    //     return middleColumnSeam(energy);
    // }
    // STUB: returns the middle column so the GUI runs end to end.
    // Replace the body with the real dynamic programming:
    //   M[0][j] = e[0][j]
    //   M[i][j] = e[i][j] + min(M[i-1][j-1], M[i-1][j], M[i-1][j+1])   (skip out-of-range parents)
    //   start at argmin_j M[H-1][j], backtrack to row 0 choosing the minimising parent.
    // Remember: ties -> leftmost column, and fill Seam::cost with the real path cost.
    // The [optimality] and [contract] tests stay red until this is done.
    // Seam DpSeamFinder::findVertical(const cv::Mat& img, const cv::Mat& energy) {
    //     Seam ret{};

    //     cv::Mat working = energy.clone();
        
    //     cv::copyMakeBorder(
    //         working, 
    //         working, 
    //         0, 0, 1, 1,     // only pad left and right 
    //         cv::BORDER_CONSTANT,
    //         std::numeric_limits<double>::infinity()
    //     );

        
    //     for(int32_t row{1}; row < working.rows; ++row){
    //         // working.at<float>(row, working.cols-1) = std::numeric_limits::float::max();
    //         // working.at<float>(row, 0) = std::numeric_limits::float::max();

    //         for(int32_t col{1}; col < working.cols-1; ++col){
    //             working.at<double>(row, col) += (
    //                 std::min(
    //                     working.at<double>(row-1, col-1),
    //                     std::min(
    //                         working.at<double>(row-1, col),
    //                         working.at<double>(row-1, col+1)
    //                     )
    //                 )
    //             );
    //         }
    //     }

    //     // cv::Rect roi(left, top, energy.cols, energy.rows);
    //     // working = working(roi).clone();

    //     int32_t col{1};
    //     for(int32_t i{2}; i < working.cols-1; ++i){
    //         col = working.at<double>(working.rows-1, col) <= working.at<double>(working.rows-1, i) ? col : i;
    //     }
    //     ret.cols.push_back(col-1); 
    //     ret.cost = working.at<double>(working.rows-1, col);

    //     auto cmp_p = [](std::pair<int, double> const& a, std::pair<int, double> const& b){
    //         return a.second < b.second;
    //     };

    //     for(int32_t row{working.rows-2}; row >= 0; --row){
    //         std::pair<int, double> 
    //             L{col-1, working.at<double>(row, col-1)},
    //             C{col, working.at<double>(row, col)},
    //             R{col+1, working.at<double>(row, col+1)}
    //         ;
    //         ret.cols.push_back((col = std::min(C, std::min(R, L, cmp_p), cmp_p).first)-1);
    //     }
    //     std::reverse(ret.cols.begin(), ret.cols.end());

    //     return ret;
    // }

    Seam DpSeamFinder::findVertical(const cv::Mat& img, const cv::Mat& energy) {
        Seam ret{};

        cv::Mat I{}, working{}, gray{};
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        gray.convertTo(I, CV_32F);

        std::vector<int> parent_mtx_back(img.rows*img.cols);

        working = cv::Mat::zeros(I.size(), I.type());
        // working.row(rowIdx).setTo(cv::Scalar(0.0f));

        cv::copyMakeBorder(
            I, 
            I, 
            0, 0, 1, 1,     // only pad left and right 
            cv::BORDER_REPLICATE
        );

        
        cv::copyMakeBorder(
            working, 
            working, 
            0, 0, 1, 1,     // only pad left and right 
            cv::BORDER_CONSTANT,
            std::numeric_limits<float>::infinity()
        );

        
        auto cmp_p = [](std::pair<int, float> const& a, std::pair<int, float> const& b){
            return a.second < b.second;
        };

        for(int32_t row{1}; row < working.rows; ++row){
            for(int32_t col{1}; col < working.cols-1; ++col){
                std::pair<int, float> 
                    OUT{
                        std::min(
                            std::pair<int, float>{0, working.at<float>(row-1, col) + std::abs(I.at<float>(row, col-1) - I.at<float>(row, col+1))},
                            std::min(
                                std::pair<int, float>{-1, working.at<float>(row-1, col-1) + std::abs(I.at<float>(row, col-1) - I.at<float>(row, col+1)) + std::abs(I.at<float>(row, col-1) - I.at<float>(row-1, col))},
                                std::pair<int, float>{+1, working.at<float>(row-1, col+1) + std::abs(I.at<float>(row, col-1) - I.at<float>(row, col+1)) + std::abs(I.at<float>(row, col+1) - I.at<float>(row-1, col))},
                                cmp_p
                            ),
                            cmp_p
                        )
                    }
                ;
                parent_mtx_back[row*(working.cols-2) + (col-1)] = OUT.first;
                working.at<float>(row, col) = OUT.second;
            }
        }

        // cv::Rect roi(left, top, energy.cols, energy.rows);
        // working = working(roi).clone();

        int32_t col{1};
        for(int32_t i{2}; i < working.cols-1; ++i){
            col = working.at<float>(working.rows-1, col) <= working.at<float>(working.rows-1, i) ? col : i;
        }
        ret.cols.push_back(col-1); 
        ret.cost = working.at<float>(working.rows-1, col);


        for(int32_t row{working.rows-1}; row > 0; --row){
            ret.cols.push_back(ret.cols.back() + parent_mtx_back[row*(working.cols-2) + ret.cols.back()]);
        }
        std::reverse(ret.cols.begin(), ret.cols.end());
assert(ret.cols.size() == static_cast<size_t>(img.rows));
        return ret;
    }

}  // namespace sc
