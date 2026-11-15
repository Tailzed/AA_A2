// A. Energy -- passes from day one.
#include <catch2/catch_test_macros.hpp>

#include "core/Energy.hpp"
#include "tests/TestUtils.hpp"

TEST_CASE("Energy of a constant image is zero", "[core][energy]") {
    cv::Mat img(8, 8, CV_8UC3, cv::Scalar(50, 120, 200));
    cv::Mat e = sc::computeEnergy(img);
    CHECK(cv::countNonZero(e) == 0);
}

TEST_CASE("Energy of a black/white step is non-zero only at the step", "[core][energy]") {
    cv::Mat img(8, 8, CV_8UC1, cv::Scalar(0));
    img(cv::Rect(4, 0, 4, 8)).setTo(255);  // columns 0-3 black, 4-7 white

    cv::Mat e = sc::computeEnergy(img);
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            INFO("row " << r << " col " << c);
            if (c == 3 || c == 4) CHECK(e.at<double>(r, c) > 0.0);
            else CHECK(e.at<double>(r, c) == 0.0);
        }
    }
}

TEST_CASE("Horizontal ramp gives energy exactly 80 in the interior", "[core][energy]") {
    // gx = (I[j+1] - I[j-1]) * (1+2+1) = 20 * 4 = 80, gy = 0.
    cv::Mat img(5, 5, CV_8UC1);
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j) img.at<uchar>(i, j) = static_cast<uchar>(10 * j);

    cv::Mat e = sc::computeEnergy(img);
    for (int i = 0; i < 5; ++i)
        for (int j = 1; j <= 3; ++j) {
            INFO("row " << i << " col " << j);
            CHECK(e.at<double>(i, j) == 80.0);
        }
}

TEST_CASE("Energy is CV_64F, same size, non-negative, whole numbers (Sobel mode)", "[core][energy]") {
    cv::Mat img = sct::noiseImage(17, 23, 7);
    cv::Mat e = sc::computeEnergy(img, sc::EnergyMode::Sobel);

    CHECK(e.type() == CV_64F);
    CHECK(e.size() == img.size());

    double mn = 0, mx = 0;
    cv::minMaxLoc(e, &mn, &mx);
    CHECK(mn >= 0.0);
    CHECK(mx <= 2040.0);

    bool allIntegers = true;
    for (int r = 0; r < e.rows; ++r)
        for (int c = 0; c < e.cols; ++c) {
            const double v = e.at<double>(r, c);
            if (v != static_cast<double>(static_cast<long long>(v))) allIntegers = false;
        }
    CHECK(allIntegers);
}

TEST_CASE("Gradient mode is also CV_64F and non-negative", "[core][energy]") {
    cv::Mat img = sct::noiseImage(10, 10, 3);
    cv::Mat e = sc::computeEnergy(img, sc::EnergyMode::Gradient);
    CHECK(e.type() == CV_64F);
    double mn = 0;
    cv::minMaxLoc(e, &mn, nullptr);
    CHECK(mn >= 0.0);
}
