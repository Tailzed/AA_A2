#include "core/SeamFinder.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "core/DpSeamFinder.hpp"
#include "core/Energy.hpp"
#include "core/GraphCutSeamFinder.hpp"
#include "core/GreedySeamFinder.hpp"
#include "core/SeamOps.hpp"

namespace sc {

bool ISeamFinder::init(const cv::Mat& src, int targetW, int targetH,
                       const CarveOptions& options, const ProgressFn& progress) {
    if (src.empty()) throw std::invalid_argument("init: empty image");
    if (src.depth() != CV_8U) throw std::invalid_argument("init: expected an 8-bit image");

    if (targetW <= 0) targetW = 1;
    if (targetH <= 0) targetH = 1;
    targetW = std::clamp(targetW, 1, src.cols);
    targetH = std::clamp(targetH, 1, src.rows);

    const int W0 = src.cols;
    const int H0 = src.rows;
    const int nV = W0 - targetW;
    const int nH = H0 - targetH;
    const int total = nV + nH;
    int done = 0;

    vMap_ = cv::Mat1i(H0, W0);
    vMap_.setTo(-1);
    hMap_ = cv::Mat1i(H0, W0);
    hMap_.setTo(-1);

    // 1. Precompute vertical seams into vMap_ (removal order 0 .. W0 - 1)
    if (nV > 0) {
        cv::Mat img = src.clone();
        cv::Mat idx(H0, W0, CV_32S);
        for (int r = 0; r < H0; ++r)
            for (int c = 0; c < W0; ++c) idx.at<int>(r, c) = r * W0 + c;

        for (int k = 0; k < nV; ++k) {
            const cv::Mat energy = computeEnergy(img, options.energy);
            Seam seam = findVertical(img, energy);
            if (!isValidSeam(seam.cols, img.rows, img.cols))
                throw std::runtime_error(name() + " returned an invalid seam during init");

            for (int r = 0; r < img.rows; ++r) {
                const int orig = idx.at<int>(r, seam.cols[r]);
                vMap_(orig / W0, orig % W0) = k;
            }
            img = removeVerticalSeam(img, seam.cols);
            idx = removeVerticalSeam(idx, seam.cols);

            ++done;
            if (progress && !progress(done, total, seam)) return false;
        }

        // If targetW == 1, the single remaining column gets index W0 - 1
        if (targetW == 1 && img.cols == 1) {
            for (int r = 0; r < H0; ++r) {
                const int orig = idx.at<int>(r, 0);
                vMap_(orig / W0, orig % W0) = W0 - 1;
            }
        }
    } else {
        vMap_.setTo(0);
    }

    // 2. Precompute horizontal seams into hMap_ via transposition (removal order 0 .. H0 - 1)
    if (nH > 0) {
        cv::Mat img = transposeImage(src);
        cv::Mat idx(W0, H0, CV_32S);
        for (int c = 0; c < W0; ++c)
            for (int r = 0; r < H0; ++r) idx.at<int>(c, r) = r * W0 + c;

        for (int k = 0; k < nH; ++k) {
            const cv::Mat energy = computeEnergy(img, options.energy);
            Seam seam = findVertical(img, energy);
            if (!isValidSeam(seam.cols, img.rows, img.cols))
                throw std::runtime_error(name() + " returned an invalid seam during init");

            for (int c = 0; c < img.rows; ++c) {
                const int orig = idx.at<int>(c, seam.cols[c]);
                hMap_(orig / W0, orig % W0) = k;
            }
            img = removeVerticalSeam(img, seam.cols);
            idx = removeVerticalSeam(idx, seam.cols);

            ++done;
            if (progress && !progress(done, total, seam)) return false;
        }

        // If targetH == 1, the single remaining row gets index H0 - 1
        if (targetH == 1 && img.cols == 1) {
            for (int c = 0; c < W0; ++c) {
                const int orig = idx.at<int>(c, 0);
                hMap_(orig / W0, orig % W0) = H0 - 1;
            }
        }
    } else {
        hMap_.setTo(0);
    }

    return true;
}

cv::Mat ISeamFinder::reconstruct(const cv::Mat& src, int targetW, int targetH) const {
    if (src.empty()) return cv::Mat();
    if (targetW <= 0 || targetH <= 0) return cv::Mat();

    targetW = std::clamp(targetW, 1, src.cols);
    targetH = std::clamp(targetH, 1, src.rows);

    if (targetW == src.cols && targetH == src.rows) return src.clone();

    const int W0 = src.cols;
    const int H0 = src.rows;
    const int kv = W0 - targetW;
    const int kh = H0 - targetH;

    cv::Mat current = src;
    cv::Mat1i curHMap = hMap_;

    // Step 1: Filter columns using vMap_ (pixels with vMap_ >= kv survive)
    if (kv > 0 && !vMap_.empty()) {
        cv::Mat out(H0, targetW, src.type());
        cv::Mat1i filteredHMap;
        if (!hMap_.empty()) filteredHMap.create(H0, targetW);

        const int elemSize = static_cast<int>(src.elemSize());
        for (int r = 0; r < H0; ++r) {
            const uchar* srcRow = src.ptr<uchar>(r);
            uchar* dstRow = out.ptr<uchar>(r);
            const int* vRow = vMap_.ptr<int>(r);
            const int* srcHRow = hMap_.empty() ? nullptr : hMap_.ptr<int>(r);
            int* dstHRow = filteredHMap.empty() ? nullptr : filteredHMap.ptr<int>(r);

            int dstCol = 0;
            for (int c = 0; c < W0 && dstCol < targetW; ++c) {
                if (vRow[c] >= kv) {
                    std::memcpy(dstRow + dstCol * elemSize, srcRow + c * elemSize, elemSize);
                    if (dstHRow && srcHRow) dstHRow[dstCol] = srcHRow[c];
                    ++dstCol;
                }
            }
        }
        current = out;
        curHMap = filteredHMap;
    }

    // Step 2: Filter rows using curHMap (top targetH ranking rows survive per column)
    if (kh > 0 && !curHMap.empty()) {
        cv::Mat out(targetH, targetW, current.type());
        const int elemSize = static_cast<int>(current.elemSize());

        std::vector<int> rowIndices(H0);
        for (int c = 0; c < targetW; ++c) {
            std::iota(rowIndices.begin(), rowIndices.end(), 0);
            std::nth_element(rowIndices.begin(), rowIndices.begin() + targetH, rowIndices.end(),
                             [&](int r1, int r2) {
                                 return curHMap(r1, c) > curHMap(r2, c);
                             });
            std::sort(rowIndices.begin(), rowIndices.begin() + targetH);

            for (int r = 0; r < targetH; ++r) {
                const int srcR = rowIndices[r];
                std::memcpy(out.ptr<uchar>(r) + c * elemSize,
                            current.ptr<uchar>(srcR) + c * elemSize,
                            elemSize);
            }
        }
        current = out;
    }

    return current;
}

cv::Mat1i ISeamFinder::buildRemovedOrder(const cv::Mat& src, int targetW, int targetH) const {
    if (src.empty()) return cv::Mat1i();
    const int W0 = src.cols;
    const int H0 = src.rows;
    targetW = std::clamp(targetW, 1, W0);
    targetH = std::clamp(targetH, 1, H0);

    const int kv = W0 - targetW;
    const int kh = H0 - targetH;

    cv::Mat1i order = cv::Mat1i::zeros(H0, W0);
    if (vMap_.empty()) return order;

    // 1. Mark vertical seams removed: 1-based order = 1 .. kv
    for (int r = 0; r < H0; ++r) {
        for (int c = 0; c < W0; ++c) {
            if (vMap_(r, c) >= 0 && vMap_(r, c) < kv) {
                order(r, c) = vMap_(r, c) + 1;
            }
        }
    }

    // 2. Mark horizontal seams removed for pixels that survived vertical carving
    if (kh > 0 && !hMap_.empty()) {
        std::vector<std::vector<int>> origCol(H0);
        for (int r = 0; r < H0; ++r) {
            for (int c = 0; c < W0; ++c) {
                if (vMap_(r, c) >= kv) {
                    origCol[r].push_back(c);
                }
            }
        }

        std::vector<int> rowIndices(H0);
        for (int c = 0; c < targetW; ++c) {
            std::iota(rowIndices.begin(), rowIndices.end(), 0);
            std::nth_element(rowIndices.begin(), rowIndices.begin() + targetH, rowIndices.end(),
                             [&](int r1, int r2) {
                                 const int oc1 = origCol[r1][c];
                                 const int oc2 = origCol[r2][c];
                                 return hMap_(r1, oc1) > hMap_(r2, oc2);
                             });

            // Rows in rowIndices[targetH ... H0 - 1] are removed by horizontal seams
            std::sort(rowIndices.begin() + targetH, rowIndices.end(),
                      [&](int r1, int r2) {
                          const int oc1 = origCol[r1][c];
                          const int oc2 = origCol[r2][c];
                          return hMap_(r1, oc1) < hMap_(r2, oc2);
                      });

            for (int i = targetH; i < H0; ++i) {
                const int r = rowIndices[i];
                const int oc = origCol[r][c];
                const int hIndex = (i - targetH);
                order(r, oc) = kv + hIndex + 1;
            }
        }
    }

    return order;
}

std::unique_ptr<ISeamFinder> makeFinder(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::DP: return std::make_unique<DpSeamFinder>();
        case Algorithm::Greedy: return std::make_unique<GreedySeamFinder>();
        case Algorithm::GraphCut: return std::make_unique<GraphCutSeamFinder>();
    }
    throw std::invalid_argument("makeFinder: unknown algorithm");
}

}  // namespace sc
