// D. Algorithm contract -- every finder (DP, Greedy, GraphCut) must pass these.
// With the shipped stubs the "canyon" test fails (the stubs return the middle column).
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <utility>

#include "core/Energy.hpp"
#include "tests/TestUtils.hpp"

TEST_CASE("Finder contract: seams are valid on awkward sizes", "[contract]") {
    const auto algo = GENERATE(sc::Algorithm::DP, sc::Algorithm::Greedy, sc::Algorithm::GraphCut);
    auto finder = sc::makeFinder(algo);
    INFO("finder = " << finder->name());

    cv::RNG rng(42);
    const std::vector<std::pair<int, int>> sizes{{1, 5}, {5, 1}, {2, 2}, {30, 40}};  // {rows, cols}
    for (const auto& [h, w] : sizes) {
        INFO("size " << h << " rows x " << w << " cols");
        cv::Mat energy = sct::randomEnergy(h, w, rng);
        cv::Mat img(h, w, CV_8UC3, cv::Scalar::all(0));
        sc::Seam seam = finder->findVertical(img, energy);

        REQUIRE(sc::isValidSeam(seam, h, w));
        // Reported cost must match the real cost of the returned path.
        CHECK(sc::seamCost(energy, seam) == seam.cost);
    }
}

TEST_CASE("Finder contract: deterministic", "[contract]") {
    const auto algo = GENERATE(sc::Algorithm::DP, sc::Algorithm::Greedy, sc::Algorithm::GraphCut);
    auto finder = sc::makeFinder(algo);
    INFO("finder = " << finder->name());

    cv::RNG rng(7);
    cv::Mat energy = sct::randomEnergy(25, 30, rng);
    cv::Mat img(25, 30, CV_8UC3, cv::Scalar::all(0));
    sc::Seam a = finder->findVertical(img, energy);
    sc::Seam b = finder->findVertical(img, energy);
    CHECK(a.cols == b.cols);
    CHECK(a.cost == b.cost);
}

TEST_CASE("Finder contract: uniform energy costs H * c", "[contract]") {
    const auto algo = GENERATE(sc::Algorithm::DP, sc::Algorithm::Greedy, sc::Algorithm::GraphCut);
    auto finder = sc::makeFinder(algo);
    INFO("finder = " << finder->name());

    const int h = 12, w = 9;
    const double c = 17.0;
    cv::Mat energy(h, w, CV_64F, cv::Scalar(c));
    cv::Mat img(h, w, CV_8UC3, cv::Scalar::all(0));
    sc::Seam seam = finder->findVertical(img, energy);
    REQUIRE(sc::isValidSeam(seam, h, w));
    CHECK(sc::seamCost(energy, seam) == h * c);
}

TEST_CASE("Finder contract: canyon -- a zero-cost column must be found", "[contract]") {
    const auto algo = GENERATE(sc::Algorithm::DP, sc::Algorithm::Greedy, sc::Algorithm::GraphCut);
    auto finder = sc::makeFinder(algo);
    INFO("finder = " << finder->name());

    const int h = 10, w = 10, canyon = 6;
    cv::Mat energy(h, w, CV_64F, cv::Scalar(100));
    for (int r = 0; r < h; ++r) energy.at<double>(r, canyon) = 0.0;
    cv::Mat img(h, w, CV_8UC3, cv::Scalar::all(0));

    sc::Seam seam = finder->findVertical(img, energy);
    REQUIRE(sc::isValidSeam(seam, h, w));
    CHECK(sc::seamCost(energy, seam) == 0.0);
    for (int r = 0; r < h; ++r) {
        INFO("row " << r);
        CHECK(seam.cols[r] == canyon);
    }
}
