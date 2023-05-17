#ifndef MIN_MAX_PYRAMID_
#define MIN_MAX_PYRAMID_

#include <algorithm>
#include <vector>

#include "opencv4/opencv2/opencv.hpp"

// Custom downsample that applies fn to the four available pixels in the output.
// We assume fn is associative, i.e.
//     fn(fn(a, b), fn(c, d)) == fn(fn(a, c), fn(b, d))
// Otherwise the result is unspecified.
//
// Also note that the last row/col are thrown away in the case of odd input
// dimensions.
//
// Exposed for testing.
cv::Mat Downsample(const cv::Mat input, std::function<uchar(uchar, uchar)> fn,
                   uchar default_value);

cv::Mat DownsampleMin(const cv::Mat input);
cv::Mat DownsampleMax(const cv::Mat input);

class MinMaxPyramid {
public:
  MinMaxPyramid() = default;

  // Pre-process the image by building the relevant pyramid
  void PreProcess(cv::Mat input);

  cv::Mat Relevel(int scale) const;

  // Smallest scale at which Relevel will operate. Smaller inputs will be
  // treated the same as the min value, so this is mostly present to improve UI.
  int MinScale() const { return min_layer_; }
  // Largest scale at which Relevel can operate. Out-of-range inputs return the
  // original image.
  int MaxScale() const { return max_layer_; }

private:
  cv::Mat image_;

  // The largest-scale layer, set during pre-processing. Default set high to
  // allow for ridiculously large images, without allowing total runaway if
  // something goes wrong.
  int max_layer_ = 50;
  // Smallest-scale layer to precompute. This is mostly a cost-saving measure,
  // but these layers aren't very useful because as the min/max gets very local,
  // Relevel degenerates into an edge detector.
  int min_layer_ = 4;

  std::vector<cv::Mat> min_images_;
  std::vector<cv::Mat> max_images_;
};
#endif // MIN_MAX_PYRAMID_
