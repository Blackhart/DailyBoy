#include "process/reformat.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <gtest/gtest.h>

#include <array>

#include "image/frame.hpp"
#include "status.hpp"

namespace {

OIIO::ImageBuf make_buf(int width, int height, int nchannels, int x, int y,
                        int full_x, int full_y, int full_width, int full_height,
                        const float* fill) {
  OIIO::ImageSpec spec(width, height, nchannels, OIIO::TypeDesc::FLOAT);
  spec.x = x;
  spec.y = y;
  spec.full_x = full_x;
  spec.full_y = full_y;
  spec.full_width = full_width;
  spec.full_height = full_height;
  OIIO::ImageBuf buf(spec);
  EXPECT_TRUE(
      OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(fill, nchannels)));
  return buf;
}

std::array<float, 3> pixel_rgb(const OIIO::ImageBuf& buf, int x, int y) {
  float px[3] = {0.0f, 0.0f, 0.0f};
  EXPECT_TRUE(
      buf.get_pixels(OIIO::ROI(x, x + 1, y, y + 1), OIIO::TypeDesc::FLOAT, px));
  return {px[0], px[1], px[2]};
}

}  // namespace

/*!
 * \brief Restores a cropped RGBA data window into an 8x8 RGB display.
 */
TEST(Reformat, Reformat_CroppedRgba_RestoresDisplayAndDropsAlpha) {
  // Prepare
  const float rgba[4] = {1.0f, 0.0f, 0.0f, 1.0f};
  dailyboy::Frame frame(make_buf(2, 2, 4, 2, 3, 0, 0, 8, 8, rgba));

  // Test
  dailyboy::Status status = dailyboy::reformat(frame);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(frame.width(), 8);
  EXPECT_EQ(frame.height(), 8);
  EXPECT_EQ(frame.buf().spec().nchannels, 3);
  EXPECT_EQ(frame.buf().spec().x, 0);
  EXPECT_EQ(frame.buf().spec().y, 0);
  const std::array<float, 3> roi = pixel_rgb(frame.buf(), 2, 3);
  EXPECT_FLOAT_EQ(roi[0], 1.0f);
  EXPECT_FLOAT_EQ(roi[1], 0.0f);
  EXPECT_FLOAT_EQ(roi[2], 0.0f);
  const std::array<float, 3> outside = pixel_rgb(frame.buf(), 0, 0);
  EXPECT_FLOAT_EQ(outside[0], 0.0f);
  EXPECT_FLOAT_EQ(outside[1], 0.0f);
  EXPECT_FLOAT_EQ(outside[2], 0.0f);
}

/*!
 * \brief Leaves an odd crop inside an even display at display size (not
 *        padded to the next even crop).
 */
TEST(Reformat, Reformat_OddCropEvenDisplay_KeepsDisplaySize) {
  // Prepare
  const float rgb[3] = {1.0f, 1.0f, 0.0f};
  dailyboy::Frame frame(make_buf(3, 3, 3, 2, 3, 0, 0, 8, 8, rgb));

  // Test
  dailyboy::Status status = dailyboy::reformat(frame);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(frame.width(), 8);
  EXPECT_EQ(frame.height(), 8);
  const std::array<float, 3> roi = pixel_rgb(frame.buf(), 2, 3);
  EXPECT_FLOAT_EQ(roi[0], 1.0f);
  EXPECT_FLOAT_EQ(roi[1], 1.0f);
  EXPECT_FLOAT_EQ(roi[2], 0.0f);
  const std::array<float, 3> outside = pixel_rgb(frame.buf(), 0, 0);
  EXPECT_FLOAT_EQ(outside[0], 0.0f);
  EXPECT_FLOAT_EQ(outside[1], 0.0f);
  EXPECT_FLOAT_EQ(outside[2], 0.0f);
}

/*!
 * \brief Leaves an odd display window at its display size (no even pad).
 */
TEST(Reformat, Reformat_OddDisplay_KeepsDisplaySize) {
  // Prepare
  const float rgb[3] = {0.0f, 1.0f, 0.0f};
  dailyboy::Frame frame(make_buf(7, 5, 3, 0, 0, 0, 0, 7, 5, rgb));

  // Test
  dailyboy::Status status = dailyboy::reformat(frame);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(frame.width(), 7);
  EXPECT_EQ(frame.height(), 5);
  EXPECT_EQ(frame.buf().spec().nchannels, 3);
  const std::array<float, 3> kept = pixel_rgb(frame.buf(), 0, 0);
  EXPECT_FLOAT_EQ(kept[0], 0.0f);
  EXPECT_FLOAT_EQ(kept[1], 1.0f);
  EXPECT_FLOAT_EQ(kept[2], 0.0f);
}

/*!
 * \brief Pastes a cropped ROI relative to a non-zero display origin.
 */
TEST(Reformat, Reformat_OffsetDisplayOrigin_PastesRelative) {
  // Prepare
  const float rgba[4] = {0.0f, 0.0f, 1.0f, 1.0f};
  dailyboy::Frame frame(make_buf(2, 2, 4, 6, 8, 4, 6, 8, 8, rgba));

  // Test
  dailyboy::Status status = dailyboy::reformat(frame);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(frame.width(), 8);
  EXPECT_EQ(frame.height(), 8);
  EXPECT_EQ(frame.buf().spec().nchannels, 3);
  const std::array<float, 3> roi = pixel_rgb(frame.buf(), 2, 2);
  EXPECT_FLOAT_EQ(roi[0], 0.0f);
  EXPECT_FLOAT_EQ(roi[1], 0.0f);
  EXPECT_FLOAT_EQ(roi[2], 1.0f);
  const std::array<float, 3> outside = pixel_rgb(frame.buf(), 0, 0);
  EXPECT_FLOAT_EQ(outside[0], 0.0f);
  EXPECT_FLOAT_EQ(outside[1], 0.0f);
  EXPECT_FLOAT_EQ(outside[2], 0.0f);
}

/*!
 * \brief Drops alpha on a full-frame even RGBA buffer without resizing.
 */
TEST(Reformat, Reformat_FullFrameRgba_DropsAlpha) {
  // Prepare
  const float rgba[4] = {0.25f, 0.5f, 0.75f, 0.4f};
  dailyboy::Frame frame(make_buf(8, 8, 4, 0, 0, 0, 0, 8, 8, rgba));

  // Test
  dailyboy::Status status = dailyboy::reformat(frame);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(frame.width(), 8);
  EXPECT_EQ(frame.height(), 8);
  EXPECT_EQ(frame.buf().spec().nchannels, 3);
  const std::array<float, 3> px = pixel_rgb(frame.buf(), 3, 4);
  EXPECT_FLOAT_EQ(px[0], 0.25f);
  EXPECT_FLOAT_EQ(px[1], 0.5f);
  EXPECT_FLOAT_EQ(px[2], 0.75f);
}

/*!
 * \brief Leaves an even RGB origin-zero plate unchanged.
 */
TEST(Reformat, Reformat_EvenRgbOriginZero_IsNoOp) {
  // Prepare
  const float rgb[3] = {0.1f, 0.2f, 0.3f};
  dailyboy::Frame frame(make_buf(8, 8, 3, 0, 0, 0, 0, 8, 8, rgb));

  // Test
  dailyboy::Status status = dailyboy::reformat(frame);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(frame.width(), 8);
  EXPECT_EQ(frame.height(), 8);
  EXPECT_EQ(frame.buf().spec().nchannels, 3);
  const std::array<float, 3> px = pixel_rgb(frame.buf(), 1, 1);
  EXPECT_FLOAT_EQ(px[0], 0.1f);
  EXPECT_FLOAT_EQ(px[1], 0.2f);
  EXPECT_FLOAT_EQ(px[2], 0.3f);
}
