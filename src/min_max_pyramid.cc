#include "min_max_pyramid.h"

#include <algorithm>
#include <cmath>

using uchar = unsigned char;

// Note: we could compute max and min for the first level in one pass, but after
// that the source images are different. Might be worth special-casing anyway as
// the base image is the largest we handle.
cv::Mat Downsample(const cv::Mat input, std::function<uchar(uchar, uchar)> fn) {
  // Pad dimensions, but only if the size of the input is odd.
  // I.e., 3- or 4-pixel wide input will map to 2 pixel output.
  const int nRows = (input.rows + 1) / 2;
  const int nCols = (input.cols + 1) / 2;

  // Walk through the image using row pointers; this is much faster than
  // accessing pixels by coordinate, though admittedly readability suffers.
  cv::Mat output(nRows, nCols, CV_8UC3);
  for (int row = 0; row < (input.rows / 2); row++) {
    uchar *outrow = output.ptr(row);
    const uchar *row1 = input.ptr(row * 2);
    const uchar *row2 = input.ptr(row * 2 + 1);
    for (int col = 0; col < (input.cols / 2); col++) {
      // Call the channels RGB for convenience, though note that OpenCV often
      // uses BGR order. We don't actually care what color we're dealing with,
      // so long as we keep the channels separate.
      uchar r, g, b;
      r = fn(*row1++, *row2++);
      g = fn(*row1++, *row2++);
      b = fn(*row1++, *row2++);
      r = fn(fn(r, *row1++), *row2++);
      g = fn(fn(g, *row1++), *row2++);
      b = fn(fn(b, *row1++), *row2++);
      *outrow++ = r;
      *outrow++ = g;
      *outrow++ = b;
    }
    // Special case the last column in case of odd column count
    if (nCols != (input.cols / 2)) {
      *outrow++ = fn(*row1++, *row2++);
      *outrow++ = fn(*row1++, *row2++);
      *outrow++ = fn(*row1++, *row2++);
    }
  }
  // Special case the last row in case of odd row count
  if (nRows != (input.rows / 2)) {
    uchar *outrow = output.ptr(input.rows / 2);
    const uchar *row = input.ptr(input.rows - 1);
    for (int col = 0; col < (input.cols / 2); col++) {
      uchar r = *row++;
      uchar g = *row++;
      uchar b = *row++;
      *outrow++ = fn(r, *row++);
      *outrow++ = fn(g, *row++);
      *outrow++ = fn(b, *row++);
    }
    // Final check for both row and col numbers being odd: in that case there's
    // only one available input pixel, so just keep it.
    if (nCols != (input.cols / 2)) {
      *outrow++ = *row++;
      *outrow++ = *row++;
      *outrow++ = *row++;
    }
  }
  return output;
}

cv::Mat DownsampleMax(cv::Mat input) {
  return Downsample(input, [](uchar a, uchar b) { return a > b ? a : b; });
}

cv::Mat DownsampleMin(cv::Mat input) {
  return Downsample(input, [](uchar a, uchar b) { return a < b ? a : b; });
}

void MinMaxPyramid::PreProcess(cv::Mat input) {
  image_ = input;

  // Clear out any previous state
  min_images_.clear();
  max_images_.clear();
  max_layer_ = 50;

  std::vector<cv::Mat> min_pyramid;
  std::vector<cv::Mat> max_pyramid;
  min_pyramid.push_back(DownsampleMin(input));
  max_pyramid.push_back(DownsampleMax(input));

  for (int i = 1; i < max_layer_; i++) {
    if (min_pyramid.back().rows < 2 && min_pyramid.back().cols < 2) {
      max_layer_ = i - 1;
      break;
    }
    min_pyramid.push_back(DownsampleMin(min_pyramid.back()));
    max_pyramid.push_back(DownsampleMax(max_pyramid.back()));
  }

  // For very small images, make sure there are still some levels available.
  // This is mostly for testing.
  min_layer_ = std::min(
      4 /*normal min level*/,
      std::max(0, static_cast<int>(std::floor(std::log2(image_.rows)) - 4)));

  // Now upsample again. The interpolation blurs the min and max images so that
  // the final transformation doesn't have hard edges.
  for (int layer = min_layer_; layer <= max_layer_; layer++) {
    cv::Mat min_image(image_.rows, image_.cols, CV_8UC3);
    cv::Mat max_image(image_.rows, image_.cols, CV_8UC3);

    cv::resize(min_pyramid[layer], min_image, min_image.size(), 0, 0,
               cv::INTER_LINEAR);
    cv::resize(max_pyramid[layer], max_image, max_image.size(), 0, 0,
               cv::INTER_LINEAR);

    min_images_.push_back(min_image);
    max_images_.push_back(max_image);
  }
}

cv::Mat MinMaxPyramid::Relevel(int scale) const {
  if (scale < min_layer_ || scale > max_layer_)
    return image_;
  cv::Mat output(image_.rows, image_.cols, CV_8UC3);

  const cv::Mat max_img = max_images_[scale - min_layer_];
  const cv::Mat min_img = min_images_[scale - min_layer_];

  cv::Mat range_img = max_img - min_img;

  cv::Mat zeroed = image_ - min_img;

  // Now the annoying bit. We should be able to write
  //     return zeroed.mul(255 / range_img);
  // but the integer division truncates, and that creates visible artifacts
  // when the cutoff changes along a gradient. So we have to do this as floating
  // point and then convert back.
  cv::Mat zeroed_f, range_f;
  zeroed.convertTo(zeroed_f, CV_32F);
  range_img.convertTo(range_f, CV_32F);
  cv::Mat resultf, result;
  resultf = zeroed_f.mul(255.f / range_f);
  resultf.convertTo(result, CV_8U);
  return result;
}
