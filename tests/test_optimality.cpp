// E. Optimality -- RED until you replace the stubs with real algorithms.
//    Run just these with:  seamcore_tests "[optimality]"
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <iostream>

#include "core/Energy.hpp"
#include "tests/TestUtils.hpp"

namespace {

// Runs `finder` on `energy` and returns the seam's REAL cost (recomputed here,
// never trusting Seam::cost). Fails the test if the seam is invalid.
double realCost(sc::ISeamFinder& finder, const cv::Mat& energy) {
    cv::Mat img(energy.rows, energy.cols, CV_8UC3, cv::Scalar::all(0));
    sc::Seam seam = finder.findVertical(img, energy);
    REQUIRE(sc::isValidSeam(seam, energy.rows, energy.cols));
    return sc::seamCost(energy, seam);
}

constexpr int kTrials = 200;
constexpr int kRows = 6;
constexpr int kCols = 5;

}  // namespace

TEST_CASE("DP finds the optimal seam (vs brute force)", "[optimality][dp]") {
    auto dp = sc::makeFinder(sc::Algorithm::DP);
    cv::RNG rng(2024);
    int bad = 0;
    cv::Mat firstBad;
    for (int t = 0; t < kTrials; ++t) {
        cv::Mat e = sct::randomEnergy(kRows, kCols, rng);
        if (realCost(*dp, e) != sct::bruteForceMin(e)) {
            if (bad++ == 0) firstBad = e.clone();
        }
    }
    INFO("first failing matrix:\n" << firstBad);
    CHECK(bad == 0);
}

TEST_CASE("Greedy is valid and never better than optimal", "[optimality][greedy]") {
    auto greedy = sc::makeFinder(sc::Algorithm::Greedy);
    cv::RNG rng(2024);
    int bad = 0;
    for (int t = 0; t < kTrials; ++t) {
        cv::Mat e = sct::randomEnergy(kRows, kCols, rng);
        if (realCost(*greedy, e) < sct::bruteForceMin(e)) ++bad;  // impossible for a valid seam
    }
    CHECK(bad == 0);
}

TEST_CASE("Graph cut finds the optimal seam (vs brute force)", "[optimality][graphcut]") {
    // NOTE: needs the right-padding fix described in GraphCutSeamFinder.hpp,
    // otherwise the seam can never use the last column and this can fail
    // through no fault of the max-flow code.
    auto gc = sc::makeFinder(sc::Algorithm::GraphCut);
    cv::RNG rng(2024);
    int bad = 0;
    cv::Mat firstBad;
    for (int t = 0; t < kTrials; ++t) {
        cv::Mat e = sct::randomEnergy(kRows, kCols, rng);
        if (realCost(*gc, e) != sct::bruteForceMin(e)) {
            if (bad++ == 0) firstBad = e.clone();
        }
    }
    INFO("first failing matrix:\n" << firstBad);
    CHECK(bad == 0);
}

TEST_CASE("Greedy trap: DP beats greedy on a hand-checked matrix", "[optimality][trap]") {
    // DP : col1 (2) -> col0 (5) -> col0 (0)  = 7
    // Greedy: col1 (2) -> col2 (1) -> forced onto a 100 = 103
    const cv::Mat e = sct::makeEnergy({{3, 2, 9, 9},
                                       {5, 5, 1, 9},
                                       {0, 100, 100, 100}});
    auto dp = sc::makeFinder(sc::Algorithm::DP);
    auto greedy = sc::makeFinder(sc::Algorithm::Greedy);
    CHECK(sct::bruteForceMin(e) == 7.0);
    CHECK(realCost(*dp, e) == 7.0);
    CHECK(realCost(*greedy, e) == 103.0);
}

TEST_CASE("DP and graph cut agree on a realistic image", "[optimality][realistic]") {
    // 64x48 image: horizontal gradient plus seeded noise.
    const int h = 48, w = 64;
    cv::Mat img(h, w, CV_8UC3);
    cv::RNG rng(99);
    for (int r = 0; r < h; ++r)
        for (int c = 0; c < w; ++c) {
            const int base = c * 3;
            for (int ch = 0; ch < 3; ++ch)
                img.at<cv::Vec3b>(r, c)[ch] = cv::saturate_cast<uchar>(base + rng.uniform(0, 40));
        }
    const cv::Mat energy = sc::computeEnergy(img);

    auto dp = sc::makeFinder(sc::Algorithm::DP);
    auto gc = sc::makeFinder(sc::Algorithm::GraphCut);
    auto greedy = sc::makeFinder(sc::Algorithm::Greedy);

    auto timeIt = [&](sc::ISeamFinder& f, double& cost) {
        const auto t0 = std::chrono::steady_clock::now();
        cost = realCost(f, energy);
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    };
    double cDp = 0, cGc = 0, cGreedy = 0;
    const double tDp = timeIt(*dp, cDp);
    const double tGc = timeIt(*gc, cGc);
    const double tGreedy = timeIt(*greedy, cGreedy);

    // Timings are informational only (copy them into your report).
    WARN("single-seam time on 64x48: DP " << tDp << " ms, GraphCut " << tGc << " ms, Greedy "
                                          << tGreedy << " ms; costs " << cDp << " / " << cGc << " / "
                                          << cGreedy);
    CHECK(cDp == cGc);
    CHECK(cGreedy >= cDp);
}
