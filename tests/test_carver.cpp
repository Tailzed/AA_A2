// C. Carver plumbing -- uses MockFinder, so it passes from day one and is
// independent of your algorithms.
#include <catch2/catch_test_macros.hpp>

#include <map>

#include "core/SeamCarver.hpp"
#include "tests/TestUtils.hpp"

TEST_CASE("carve produces the requested sizes", "[core][carver]") {
    const int H = 15, W = 20;
    cv::Mat img = sct::noiseImage(H, W, 1);
    sct::MockFinder f;

    auto r1 = sc::carve(img, W - 4, H, f);
    CHECK(r1.image.cols == W - 4);
    CHECK(r1.image.rows == H);
    CHECK(r1.image.type() == img.type());
    CHECK(r1.seamsRemoved == 4);

    auto r2 = sc::carve(img, W, H - 3, f);
    CHECK(r2.image.cols == W);
    CHECK(r2.image.rows == H - 3);
    CHECK(r2.seamsRemoved == 3);

    auto r3 = sc::carve(img, W - 5, H - 2, f);
    CHECK(r3.image.cols == W - 5);
    CHECK(r3.image.rows == H - 2);
    CHECK(r3.seamsRemoved == 7);
    CHECK_FALSE(r3.cancelled);
}

TEST_CASE("carve to the source size is a no-op", "[core][carver]") {
    cv::Mat img = sct::noiseImage(8, 9, 2);
    sct::MockFinder f;
    int callbacks = 0;
    auto r = sc::carve(img, 9, 8, f, {}, [&](int, int, const sc::Seam&) { ++callbacks; return true; });
    CHECK(sct::sameMat(r.image, img));
    CHECK(r.seamsRemoved == 0);
    CHECK(callbacks == 0);
    CHECK(f.calls == 0);
}

TEST_CASE("carve does not modify its input", "[core][carver]") {
    cv::Mat img = sct::noiseImage(8, 9, 2);
    const cv::Mat copy = img.clone();
    sct::MockFinder f;
    sc::carve(img, 5, 5, f);
    CHECK(sct::sameMat(img, copy));
}

TEST_CASE("carve rejects invalid targets", "[core][carver]") {
    cv::Mat img = sct::noiseImage(8, 9, 2);
    sct::MockFinder f;
    CHECK_THROWS_AS(sc::carve(img, 10, 8, f), std::invalid_argument);  // wider than source
    CHECK_THROWS_AS(sc::carve(img, 9, 9, f), std::invalid_argument);   // taller than source
    CHECK_THROWS_AS(sc::carve(img, 0, 8, f), std::invalid_argument);
    CHECK_THROWS_AS(sc::carve(cv::Mat(), 1, 1, f), std::invalid_argument);
}

TEST_CASE("Returning false from the callback cancels the carve", "[core][carver]") {
    cv::Mat img = sct::noiseImage(10, 20, 3);
    sct::MockFinder f;
    int calls = 0;
    auto r = sc::carve(img, 10, 10, f, {}, [&](int, int, const sc::Seam&) { return ++calls < 3; });
    CHECK(calls == 3);
    CHECK(r.cancelled);
    CHECK(r.seamsRemoved == 3);
    CHECK(r.image.cols == 17);
    CHECK(r.image.rows == 10);
}

TEST_CASE("Progress callback is called once per seam with done = 1..N", "[core][carver]") {
    cv::Mat img = sct::noiseImage(12, 14, 4);
    sct::MockFinder f;
    std::vector<int> dones, totals;
    sc::carve(img, 14 - 3, 12 - 2, f, {}, [&](int d, int t, const sc::Seam& s) {
        dones.push_back(d);
        totals.push_back(t);
        CHECK_FALSE(s.cols.empty());
        return true;
    });
    REQUIRE(dones.size() == 5);
    for (int i = 0; i < 5; ++i) {
        CHECK(dones[i] == i + 1);
        CHECK(totals[i] == 5);
    }
    CHECK(f.calls == 5);
}

TEST_CASE("removedOrder labels every removed pixel exactly once", "[core][carver]") {
    const int H = 11, W = 16, k = 5;
    cv::Mat img = sct::noiseImage(H, W, 5);
    sct::MockFinder f;
    auto r = sc::carve(img, W - k, H, f);

    REQUIRE(r.removedOrder.size() == img.size());
    CHECK(cv::countNonZero(r.removedOrder) == k * H);

    std::map<int, int> perLabel;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (r.removedOrder(y, x) > 0) ++perLabel[r.removedOrder(y, x)];
    REQUIRE(static_cast<int>(perLabel.size()) == k);
    for (int label = 1; label <= k; ++label) {
        INFO("label " << label);
        CHECK(perLabel[label] == H);
    }
}

TEST_CASE("removedOrder is consistent with the horizontal phase too", "[core][carver]") {
    const int H = 11, W = 16, kv = 3, kh = 2;
    cv::Mat img = sct::noiseImage(H, W, 6);
    sct::MockFinder f;
    auto r = sc::carve(img, W - kv, H - kh, f);

    // Vertical seams remove H pixels each; horizontal seams run on the already
    // narrower image, so they remove (W - kv) pixels each.
    CHECK(cv::countNonZero(r.removedOrder) == kv * H + kh * (W - kv));
    // The kept pixels are exactly the pixels of the output image.
    CHECK(cv::countNonZero(r.removedOrder == 0) == r.image.rows * r.image.cols);
}

TEST_CASE("Vertical-then-horizontal carve equals transposed carve", "[core][carver]") {
    const int H = 15, W = 20, k = 4;
    cv::Mat img = sct::noiseImage(H, W, 9);
    sct::MockFinder f;

    auto a = sc::carve(img, W, H - k, f);
    auto b = sc::carve(sc::transposeImage(img), H - k, W, f);
    CHECK(sct::sameMat(a.image, sc::transposeImage(b.image)));
}
