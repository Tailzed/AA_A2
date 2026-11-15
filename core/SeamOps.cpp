#include "core/SeamOps.hpp"

#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <opencv2/imgproc.hpp>

namespace sc {

bool isValidSeam(const std::vector<int>& cols, int rows, int width) {
    if (width < 1 || static_cast<int>(cols.size()) != rows) return false;
    for (int i = 0; i < rows; ++i) {
        if (cols[i] < 0 || cols[i] >= width) return false;
        if (i > 0 && std::abs(cols[i] - cols[i - 1]) > 1) return false;
    }
    return true;
}

double seamCost(const cv::Mat& energy, const std::vector<int>& cols) {
    if (energy.type() != CV_64F || static_cast<int>(cols.size()) != energy.rows)
        throw std::invalid_argument("seamCost: energy must be CV_64F with one seam entry per row");
    double sum = 0.0;
    for (int i = 0; i < energy.rows; ++i) {
        if (cols[i] < 0 || cols[i] >= energy.cols)
            throw std::invalid_argument("seamCost: column out of range");
        sum += energy.at<double>(i, cols[i]);
    }
    return sum;
}

Seam middleColumnSeam(const cv::Mat& energy) {
    Seam s;
    s.cols.assign(energy.rows, energy.cols / 2);
    s.cost = seamCost(energy, s.cols);
    return s;
}

cv::Mat removeVerticalSeam(const cv::Mat& img, const std::vector<int>& cols) {
    const int rows = img.rows;
    const int width = img.cols;
    if (width < 2) throw std::invalid_argument("removeVerticalSeam: image is only one column wide");
    if (static_cast<int>(cols.size()) != rows)
        throw std::invalid_argument("removeVerticalSeam: seam length != image height");

    cv::Mat out(rows, width - 1, img.type());
    const size_t es = img.elemSize();
    for (int r = 0; r < rows; ++r) {
        const int c = cols[r];
        if (c < 0 || c >= width) throw std::invalid_argument("removeVerticalSeam: column out of range");
        const uchar* src = img.ptr<uchar>(r);
        uchar* dst = out.ptr<uchar>(r);
        std::memcpy(dst, src, static_cast<size_t>(c) * es);
        std::memcpy(dst + static_cast<size_t>(c) * es, src + static_cast<size_t>(c + 1) * es,
                    static_cast<size_t>(width - c - 1) * es);
    }
    return out;
}

cv::Mat transposeImage(const cv::Mat& img) {
    cv::Mat t;
    cv::transpose(img, t);
    return t;
}

cv::Mat overlaySeams(const cv::Mat& original, const cv::Mat1i& removedOrder, int upTo,
                     const cv::Scalar& color) {
    if (original.size() != removedOrder.size())
        throw std::invalid_argument("overlaySeams: size mismatch");
    cv::Mat out;
    if (original.channels() == 1) cv::cvtColor(original, out, cv::COLOR_GRAY2BGR);
    else if (original.channels() == 4) cv::cvtColor(original, out, cv::COLOR_BGRA2BGR);
    else out = original.clone();

    const cv::Vec3b paint(cv::saturate_cast<uchar>(color[0]), cv::saturate_cast<uchar>(color[1]),
                          cv::saturate_cast<uchar>(color[2]));
    for (int r = 0; r < out.rows; ++r) {
        for (int c = 0; c < out.cols; ++c) {
            const int k = removedOrder(r, c);
            if (k > 0 && k <= upTo) out.at<cv::Vec3b>(r, c) = paint;
        }
    }
    return out;
}

}  // namespace sc
