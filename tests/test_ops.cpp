// B. SeamOps -- passes from day one.
#include <catch2/catch_test_macros.hpp>

#include "tests/TestUtils.hpp"

TEST_CASE("isValidSeam accepts good seams and rejects bad ones", "[core][ops]") {
    using V = std::vector<int>;
    CHECK(sc::isValidSeam(V{2, 2, 2, 2}, 4, 5));      // straight
    CHECK(sc::isValidSeam(V{1, 2, 1, 0}, 4, 5));      // zig-zag
    CHECK_FALSE(sc::isValidSeam(V{1, 2, 1}, 4, 5));   // wrong length
    CHECK_FALSE(sc::isValidSeam(V{1, 5, 4, 4}, 4, 5));// column out of range
    CHECK_FALSE(sc::isValidSeam(V{-1, 0, 0, 0}, 4, 5));
    CHECK_FALSE(sc::isValidSeam(V{0, 2, 2, 2}, 4, 5));// jump of 2 columns
}

TEST_CASE("seamCost matches a hand-computed value", "[core][ops]") {
    const cv::Mat e = sct::makeEnergy({{1, 2, 3},
                                       {4, 5, 6},
                                       {7, 8, 9}});
    CHECK(sc::seamCost(e, std::vector<int>{0, 0, 0}) == 1 + 4 + 7);
    CHECK(sc::seamCost(e, std::vector<int>{1, 2, 1}) == 2 + 6 + 8);
    CHECK(sc::seamCost(e, std::vector<int>{2, 1, 0}) == 3 + 5 + 7);
    CHECK_THROWS_AS(sc::seamCost(e, std::vector<int>{0, 0}), std::invalid_argument);
}

TEST_CASE("removeVerticalSeam removes exactly the seam pixels", "[core][ops]") {
    // Pixel value == its column index.
    cv::Mat img(4, 6, CV_32SC1);
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 6; ++c) img.at<int>(r, c) = c;

    const std::vector<int> seam{2, 3, 3, 2};
    cv::Mat out = sc::removeVerticalSeam(img, seam);

    REQUIRE(out.rows == 4);
    REQUIRE(out.cols == 5);
    for (int r = 0; r < 4; ++r) {
        std::vector<int> expected;
        for (int c = 0; c < 6; ++c)
            if (c != seam[r]) expected.push_back(c);  // order preserved
        for (int c = 0; c < 5; ++c) {
            INFO("row " << r << " col " << c);
            CHECK(out.at<int>(r, c) == expected[c]);
        }
    }
}

TEST_CASE("removeVerticalSeam works for 3-channel images and rejects bad input", "[core][ops]") {
    cv::Mat img = sct::noiseImage(5, 7, 11);
    cv::Mat out = sc::removeVerticalSeam(img, std::vector<int>(5, 0));  // remove first column
    CHECK(out.type() == img.type());
    CHECK(sct::sameMat(out, img(cv::Rect(1, 0, 6, 5)).clone()));
    CHECK_THROWS_AS(sc::removeVerticalSeam(img, std::vector<int>(4, 0)), std::invalid_argument);
    CHECK_THROWS_AS(sc::removeVerticalSeam(img, std::vector<int>(5, 7)), std::invalid_argument);
}

TEST_CASE("Transposing twice returns the original", "[core][ops]") {
    cv::Mat img = sct::noiseImage(9, 13, 5);
    cv::Mat t = sc::transposeImage(img);
    CHECK(t.rows == 13);
    CHECK(t.cols == 9);
    CHECK(sct::sameMat(sc::transposeImage(t), img));
}

TEST_CASE("overlaySeams paints exactly the removed pixels up to the requested seam", "[core][ops]") {
    cv::Mat img(3, 3, CV_8UC3, cv::Scalar(10, 20, 30));
    cv::Mat1i order = cv::Mat1i::zeros(3, 3);
    order(0, 0) = 1;
    order(1, 1) = 2;
    order(2, 2) = 3;

    cv::Mat all = sc::overlaySeams(img, order);
    CHECK(all.at<cv::Vec3b>(0, 0) == cv::Vec3b(0, 0, 255));
    CHECK(all.at<cv::Vec3b>(2, 2) == cv::Vec3b(0, 0, 255));
    CHECK(all.at<cv::Vec3b>(0, 1) == cv::Vec3b(10, 20, 30));

    cv::Mat first = sc::overlaySeams(img, order, 1);
    CHECK(first.at<cv::Vec3b>(0, 0) == cv::Vec3b(0, 0, 255));
    CHECK(first.at<cv::Vec3b>(2, 2) == cv::Vec3b(10, 20, 30));
}
