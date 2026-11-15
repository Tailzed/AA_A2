#pragma once
#include "core/SeamFinder.hpp"

namespace sc {

// Shrinks `src` (8-bit, 1/3/4 channels) to targetW x targetH.
//
// Order: all vertical seams first, then the image is transposed and the horizontal
// seams are removed as vertical ones. Energy is recomputed from scratch after every
// removal. Synchronous and single-threaded; the GUI runs it on a worker thread.
//
// Throws std::invalid_argument if the target is < 1 or larger than the source
// (enlarging via seam insertion is not implemented) or the source is empty/not 8-bit.
// Throws std::runtime_error if a finder returns an invalid seam.
//
// `progress` is called after each removed seam; returning false stops the carve
// (result.cancelled = true, image is whatever has been removed so far).
CarveResult carve(const cv::Mat& src, int targetW, int targetH, ISeamFinder& finder,
                  const CarveOptions& options = CarveOptions(), const ProgressFn& progress = ProgressFn());

}  // namespace sc
