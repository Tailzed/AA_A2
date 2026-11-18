#pragma once
#include <memory>
#include <string>

#include "core/Types.hpp"

namespace sc {

// Strategy interface: DP, Greedy and Graph-cut all implement this, so carve()
// has no per-algorithm branches and timing comparisons go through identical code.
class ISeamFinder {
public:
    virtual ~ISeamFinder() = default;
    virtual std::string name() const = 0;

    // img    : CV_8UC3 (BGR), H x W. Backward-energy finders can ignore it; a
    //          forward-energy finder needs it.
    // energy : CV_64F, H x W, non-negative, integer-valued in EnergyMode::Sobel.
    // Returns a VERTICAL seam (one column per row). Horizontal seams are handled by
    // the carver, which transposes both inputs before calling.
    //
    // Contract (checked by tests/test_contract.cpp):
    //   * the seam is valid (see isValidSeam) and deterministic
    //   * Seam::cost == sum of energy along the returned seam
    //   * ties are broken towards the LEFTMOST column
    virtual Seam findVertical(const cv::Mat& img, const cv::Mat& energy) = 0;

    // --- Precomputed Index Maps (32-bit int per pixel) ---
    const cv::Mat1i& vMap() const { return vMap_; }
    const cv::Mat1i& hMap() const { return hMap_; }
    cv::Mat1i& vMap() { return vMap_; }
    cv::Mat1i& hMap() { return hMap_; }
    bool hasMaps() const { return !vMap_.empty() || !hMap_.empty(); }
    void clearMaps() { vMap_.release(); hMap_.release(); }

    // Precomputes and populates vMap_ and hMap_.
    // targetW and targetH define how far to precompute (defaults to 1x1 if <= 0).
    // Returns false if cancelled via the progress callback.
    virtual bool init(const cv::Mat& src, int targetW = 0, int targetH = 0,
                      const CarveOptions& options = {}, const ProgressFn& progress = nullptr);

    // Fast O(H * W) reconstruction using vMap_ and hMap_ without re-running seam search
    virtual cv::Mat reconstruct(const cv::Mat& src, int targetW, int targetH) const;

    // Builds the 1-based removedOrder map for overlaySeams visualization:
    // 0 = kept, 1..kv = removed by vertical seam, kv+1..kv+kh = removed by horizontal seam
    virtual cv::Mat1i buildRemovedOrder(const cv::Mat& src, int targetW, int targetH) const;

protected:
    cv::Mat1i vMap_;  // Vertical removal order (size H0 x W0)
    cv::Mat1i hMap_;  // Horizontal removal order (size H0 x W0)
};

std::unique_ptr<ISeamFinder> makeFinder(Algorithm algorithm);

}  // namespace sc
