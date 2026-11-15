#include "core/SeamCarver.hpp"

#include <chrono>
#include <stdexcept>

#include "core/Energy.hpp"
#include "core/SeamOps.hpp"

namespace sc {

CarveResult carve(const cv::Mat& src, int targetW, int targetH, ISeamFinder& finder,
                  const CarveOptions& options, const ProgressFn& progress) {
    if (src.empty()) throw std::invalid_argument("carve: empty image");
    if (src.depth() != CV_8U) throw std::invalid_argument("carve: expected an 8-bit image");
    if (targetW < 1 || targetH < 1) throw std::invalid_argument("carve: target size must be >= 1");
    if (targetW > src.cols || targetH > src.rows)
        throw std::invalid_argument("carve: enlarging is not supported (target larger than source)");

    const auto t0 = std::chrono::steady_clock::now();

    const int W0 = src.cols;
    CarveResult res;
    res.removedOrder = cv::Mat1i::zeros(src.rows, src.cols);

    cv::Mat img = src.clone();

    // idx holds, for every pixel of the current image, its linear index in the
    // ORIGINAL image. It is carved and transposed together with the image, so the
    // seams can be stamped onto the original-size removedOrder map.
    cv::Mat idx(src.rows, src.cols, CV_32S);
    for (int r = 0; r < src.rows; ++r)
        for (int c = 0; c < src.cols; ++c) idx.at<int>(r, c) = r * W0 + c;

    const int nV = src.cols - targetW;
    const int nH = src.rows - targetH;
    const int total = nV + nH;
    int done = 0;

    // Removes n vertical seams from the current (possibly transposed) image.
    // Returns false if the progress callback asked to cancel.
    auto removeSeams = [&](int n) -> bool {
        for (int k = 0; k < n; ++k) {
            const cv::Mat energy = computeEnergy(img, options.energy);
            Seam seam = finder.findVertical(img, energy);
            if (!isValidSeam(seam.cols, img.rows, img.cols))
                throw std::runtime_error(finder.name() + " returned an invalid seam");

            ++done;
            for (int r = 0; r < img.rows; ++r) {
                const int orig = idx.at<int>(r, seam.cols[r]);
                res.removedOrder(orig / W0, orig % W0) = done;
            }
            img = removeVerticalSeam(img, seam.cols);
            idx = removeVerticalSeam(idx, seam.cols);

            if (progress && !progress(done, total, seam)) return false;
        }
        return true;
    };

    bool ok = removeSeams(nV);
    if (ok && nH > 0) {
        img = transposeImage(img);
        idx = transposeImage(idx);
        ok = removeSeams(nH);
        img = transposeImage(img);
    }

    res.image = img;
    res.seamsRemoved = done;
    res.cancelled = !ok;
    res.elapsedMs =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return res;
}

}  // namespace sc
