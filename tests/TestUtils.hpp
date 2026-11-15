#pragma once
// Helpers shared by all test files. Production code is never touched from here
// except through the public core API.

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>

#include <opencv2/core.hpp>

#include "core/SeamFinder.hpp"
#include "core/SeamOps.hpp"

namespace sct {

// Builds a CV_64F matrix from nested initializer lists of ints.
inline cv::Mat makeEnergy(std::initializer_list<std::initializer_list<int>> rows) {
    const int h = static_cast<int>(rows.size());
    const int w = static_cast<int>(rows.begin()->size());
    cv::Mat m(h, w, CV_64F);
    int r = 0;
    for (const auto& row : rows) {
        int c = 0;
        for (int v : row) m.at<double>(r, c++) = v;
        ++r;
    }
    return m;
}

// Integer-valued random energy in [0, maxv].
inline cv::Mat randomEnergy(int h, int w, cv::RNG& rng, int maxv = 255) {
    cv::Mat m(h, w, CV_64F);
    for (int r = 0; r < h; ++r)
        for (int c = 0; c < w; ++c) m.at<double>(r, c) = rng.uniform(0, maxv + 1);
    return m;
}

// Random BGR image, deterministic for a given seed.
inline cv::Mat noiseImage(int h, int w, uint64 seed = 1) {
    cv::Mat img(h, w, CV_8UC3);
    cv::RNG rng(seed);
    rng.fill(img, cv::RNG::UNIFORM, 0, 256);
    return img;
}

// Exhaustive minimum seam cost (oracle for small matrices only: 3^(H-1) * W paths).
// maxCol < 0 means "all columns allowed".
inline double bruteForceMin(const cv::Mat& e, int maxCol = -1) {
    const int hi = (maxCol < 0) ? e.cols - 1 : maxCol;
    double best = std::numeric_limits<double>::infinity();
    std::function<void(int, int, double)> rec = [&](int row, int col, double acc) {
        acc += e.at<double>(row, col);
        if (row == e.rows - 1) {
            best = std::min(best, acc);
            return;
        }
        for (int d = -1; d <= 1; ++d) {
            const int c = col + d;
            if (c >= 0 && c <= hi) rec(row + 1, c, acc);
        }
    };
    for (int c = 0; c <= hi; ++c) rec(0, c, 0.0);
    return best;
}

// Always returns the middle column. Used to test the carver plumbing independently
// of any real algorithm.
class MockFinder : public sc::ISeamFinder {
public:
    std::string name() const override { return "Mock"; }
    sc::Seam findVertical(const cv::Mat&, const cv::Mat& energy) override {
        ++calls;
        return sc::middleColumnSeam(energy);
    }
    int calls = 0;
};

// Pixel-exact equality (size, type, content).
inline bool sameMat(const cv::Mat& a, const cv::Mat& b) {
    if (a.size() != b.size() || a.type() != b.type()) return false;
    cv::Mat diff = a.reshape(1) != b.reshape(1);
    return cv::countNonZero(diff) == 0;
}

}  // namespace sct
