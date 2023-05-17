#include "min_max_pyramid.h"

#include "opencv4/opencv2/opencv.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

// Custom matcher code, courtesy of ChatGPT :-)
class ImageEqMatcher : public ::testing::MatcherInterface<cv::Mat> {
public:
  explicit ImageEqMatcher(const cv::Mat &rhs) : rhs_(rhs) {}

  bool MatchAndExplain(cv::Mat mat1,
                       ::testing::MatchResultListener *listener) const {
    if (mat1.size() != rhs_.size() || mat1.type() != rhs_.type()) {
      *listener << "size or type not match";
      return false;
    }

    bool match = true;
    for (int i = 0; i < mat1.rows; i++) {
      for (int j = 0; j < mat1.cols; j++) {
        cv::Vec3b pixel1 = mat1.at<cv::Vec3b>(i, j);
        cv::Vec3b pixel2 = rhs_.at<cv::Vec3b>(i, j);
        for (int k = 0; k < 3; k++) {
          if (pixel1[k] != pixel2[k]) {
            match = false;
            break;
          }
        }
      }
    }

    if (!match) {
      *listener << "element not match";
    }

    return match;
  }

  void DescribeTo(std::ostream *os) const { *os << "matches " << rhs_; }

  void DescribeNegationTo(std::ostream *os) const {
    *os << "does not match " << rhs_;
  }

private:
  cv::Mat rhs_;
};

inline ::testing::Matcher<cv::Mat> ImageEq(const cv::Mat &rhs) {
  return ::testing::MakeMatcher(new ImageEqMatcher(rhs));
}

TEST(MinMaxPyramid, DownsamplePreservesConstantValue) {
  // Testing to ensure that channel values don't bleed, aka R B and G are
  // processed separately.
  cv::Mat input(2, 2, CV_8UC3, cv::Scalar(1, 2, 3));
  cv::Mat output = DownsampleMin(input);

  cv::Mat expected(1, 1, CV_8UC3, cv::Scalar(1, 2, 3));
  EXPECT_THAT(output, ImageEq(expected));
}

TEST(MinMaxPyramid, DownsamplesWithMax) {
  cv::Mat input =
      (cv::Mat_<cv::Vec3b>(2, 2) << cv::Vec3b(1, 2, 3), cv::Vec3b(4, 5, 6),
       cv::Vec3b(7, 8, 9), cv::Vec3b(10, 11, 12));
  ASSERT_EQ(input.rows, 2);
  ASSERT_EQ(input.cols, 2);

  cv::Mat output = DownsampleMax(input);

  cv::Mat expected = cv::Mat_<cv::Vec3b>(1, 1) << cv::Vec3b(10, 11, 12);
  EXPECT_THAT(output, ImageEq(expected));
}

TEST(MinMaxPyramid, DownsamplesWithMin) {
  cv::Mat input =
      (cv::Mat_<cv::Vec3b>(2, 2) << cv::Vec3b(1, 2, 3), cv::Vec3b(4, 5, 6),
       cv::Vec3b(7, 8, 9), cv::Vec3b(10, 11, 12));

  cv::Mat output = DownsampleMin(input);

  cv::Mat expected = cv::Mat_<cv::Vec3b>(1, 1) << cv::Vec3b(1, 2, 3);
  EXPECT_THAT(output, ImageEq(expected));
}

TEST(MinMaxPyramid, DownsampleUpsizesForOddDimensions) {
  cv::Mat input =
      (cv::Mat_<cv::Vec3b>(3, 3) << cv::Vec3b(1, 2, 3), cv::Vec3b(4, 5, 6),
       cv::Vec3b(40, 50, 60), cv::Vec3b(7, 8, 9), cv::Vec3b(10, 11, 12),
       cv::Vec3b(100, 110, 120), cv::Vec3b(70, 80, 90), cv::Vec3b(71, 81, 91),
       cv::Vec3b(101, 111, 121));
  ASSERT_EQ(input.rows, 3);
  ASSERT_EQ(input.cols, 3);

  cv::Mat output = DownsampleMax(input);

  cv::Mat expected = (cv::Mat_<cv::Vec3b>(2, 2) << cv::Vec3b(10, 11, 12),
                      cv::Vec3b(100, 110, 120), cv::Vec3b(71, 81, 91),
                      cv::Vec3b(101, 111, 121));
  EXPECT_THAT(output, ImageEq(expected));
}

TEST(MinMaxPyramid, ReducesLevelsForSmallImage) {
  MinMaxPyramid pyramid;
  cv::Mat input =
      (cv::Mat_<cv::Vec3b>(2, 2) << cv::Vec3b(1, 2, 3), cv::Vec3b(4, 5, 6),
       cv::Vec3b(7, 8, 9), cv::Vec3b(10, 11, 12));
  pyramid.PreProcess(input);
  EXPECT_EQ(0, pyramid.MaxScale());
}

TEST(MinMaxPyramid, RelevelScalesImagePixels) {
  // We set up low and high pixels with several different ranges and scales. The
  // low value should always go to zero; the high should go to 255.
  MinMaxPyramid pyramid;
  // clang-format off
  cv::Mat input = (cv::Mat_<cv::Vec3b>(4, 4) <<
      cv::Vec3b(100, 10, 100), cv::Vec3b(100, 10, 100),
      cv::Vec3b(100, 10, 100), cv::Vec3b(100, 10, 100),
      cv::Vec3b(100, 10, 100), cv::Vec3b(100, 10, 100),
      cv::Vec3b(200, 200, 120), cv::Vec3b(200, 200, 120),
      cv::Vec3b(200, 200, 120), cv::Vec3b(200, 200, 120),
      cv::Vec3b(200, 200, 120), cv::Vec3b(200, 200, 120),
      cv::Vec3b(100, 10, 100), cv::Vec3b(100, 10, 100),
      cv::Vec3b(200, 200, 120), cv::Vec3b(200, 200, 120));

  pyramid.PreProcess(input);
  EXPECT_EQ(1, pyramid.MaxScale());

  cv::Mat expected =
      (cv::Mat_<cv::Vec3b>(4, 4) <<
       cv::Vec3b(0, 0, 0), cv::Vec3b(0, 0, 0),
       cv::Vec3b(0, 0, 0), cv::Vec3b(0, 0, 0),
       cv::Vec3b(0, 0, 0), cv::Vec3b(0, 0, 0),
       cv::Vec3b(255, 255, 255), cv::Vec3b(255, 255, 255),
       cv::Vec3b(255, 255, 255), cv::Vec3b(255, 255, 255),
       cv::Vec3b(255, 255, 255), cv::Vec3b(255, 255, 255),
       cv::Vec3b(0, 0, 0), cv::Vec3b(0, 0, 0),
       cv::Vec3b(255, 255, 255), cv::Vec3b(255, 255, 255));
  // clang-format on

  EXPECT_THAT(pyramid.Relevel(1), ImageEq(expected));
}
